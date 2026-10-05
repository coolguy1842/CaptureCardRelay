#ifdef ENABLE_PIPEWIRE
#include <application.hpp>
#include <format>
#include <memory>
#include <string>
#include <utility>

namespace pw = pipewire;

static std::string extractName(const std::string& value) {
    const auto delim = value.find(':');
    if(delim == std::string::npos) {
        return {};
    }

    const auto start = delim + 2;
    const auto end   = value.rfind('"');

    return value.substr(start, end - start);
}

std::string getNodeName(pipewire::node& node) {
    std::string name = node.props()["node.description"];
    if(name.empty()) {
        name = node.props()["node.nick"];
    }

    if(name.empty()) {
        name = node.props()["node.name"];
    }

    return name;
}

int Application::onPipewireMetadataProperty(const char* key, pipewire::metadata_property property) {
    if(strcmp(key, "default.audio.sink") == 0) {
        m_pipewire.defaultSinkName = extractName(property.value);
        updatePipewireLink();
    }

    return 0;
}

void Application::onPipewireGlobal(const pipewire::global& global) {
    if(global.type == pw::metadata::type) {
        auto metadata = m_pipewire.core->wait(m_pipewire.registry->bind<pw::metadata>(global.id));
        if(!metadata) {
            return;
        }

        auto props      = metadata->props();
        auto properties = metadata->properties();
        if(props["metadata.name"] != "default") {
            return;
        }

        m_pipewire.metadata        = std::move(metadata.value());
        m_pipewire.defaultSinkName = extractName(properties["default.audio.sink"].value);
    }

    if(global.type == pw::node::type) {
        auto nodeOpt = m_pipewire.core->wait(m_pipewire.registry->bind<pw::node>(global.id));

        if(!nodeOpt) {
            return;
        }

        pipewire::node node    = std::move(nodeOpt.value());
        const std::string name = getNodeName(node);

        if(node.props()["media.class"] == "Audio/Sink") {
            m_pipewire.sinks.push_back({
                .name = name,
                .id   = node.id(),
                .node = std::move(node),
            });
        }
        else if(node.props()["media.class"] == "Audio/Source") {
            m_pipewire.sources.push_back({
                .name = name,
                .id   = node.id(),
                .node = std::move(node),
            });

            if(m_settings.getSelectedRecordingDevice() == name && m_pipewire.currentSource != nullptr && m_pipewire.currentSource->name != name) {
                updatePipewireLink();
            }
        }
    }

    if(global.type == pw::port::type) {
        auto port = m_pipewire.core->wait(m_pipewire.registry->bind<pw::port>(global.id));
        if(!port) {
            return;
        }

        m_pipewire.ports.push_back(std::move(port.value()));
    }
}

void Application::onPipewireGlobalRemoved(uint32_t id) {
    bool removed = false;

    for(auto it = m_pipewire.sinks.begin(); it != m_pipewire.sinks.end(); it++) {
        if(it->node.id() == id) {
            m_pipewire.sinks.erase(it);
            removed = true;

            break;
        }
    }

    if(!removed) {
        for(auto it = m_pipewire.sources.begin(); it != m_pipewire.sources.end(); it++) {
            if(it->node.id() == id) {
                m_pipewire.sources.erase(it);
                removed = true;
                break;
            }
        }
    }

    if((m_pipewire.currentSource != nullptr && m_pipewire.currentSource->id == id) || m_pipewire.lastSinkID == id) {
        updatePipewireLink();
    }
}

void Application::initPipewire() {
    try {
        m_pipewire.loop    = pw::main_loop::create().value();
        m_pipewire.context = pw::context::create(m_pipewire.loop).value();
        m_pipewire.core    = pw::core::create(m_pipewire.context).value();

        auto registry = pw::registry::create(m_pipewire.core);
        if(!registry) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create pipewire registry: %s", registry.error().message().c_str());
            goto cleanup;
        }

        m_pipewire.registry         = std::move(registry.value());
        m_pipewire.registryListener = std::make_shared<pipewire::registry_listener>(m_pipewire.registry->get());
        m_pipewire.registryListener->on<pw::registry_event::global>([this](const pipewire::global& global) { this->onPipewireGlobal(global); });
        m_pipewire.registryListener->on<pw::registry_event::global_removed>([this](uint32_t id) { this->onPipewireGlobalRemoved(id); });

        m_pipewire.core->run_once();
        if(m_pipewire.defaultSinkName.empty()) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get default playback device, using SDL3");
            goto cleanup;
        }

        if(m_pipewire.metadata) {
            m_pipewire.metadataListener = std::make_unique<pw::metadata_listener>(m_pipewire.metadata.value());
            m_pipewire.metadataListener->on<pw::metadata_event::property>([this](const char* key, pw::metadata_property property) { return onPipewireMetadataProperty(key, property); });

            m_pipewire.core->run_once();
        }

        const auto prev_ports = m_pipewire.ports.size();

        auto factoryProps = pw::properties::create();
        factoryProps.set("monitor.channel-volumes", "true");

        auto virtualMic = m_pipewire.core->wait(m_pipewire.core->create(pw::null_factory{
            .type      = pw::null_factory::kind::source,
            .name      = "CaptureCardRelay",
            .positions = { "FL", "FR" },
            .props     = std::move(factoryProps),
        }));

        if(!virtualMic) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create null sink");
            goto cleanup;
        }

        m_pipewire.virtualMic.node = std::move(virtualMic.value());
        while(m_pipewire.ports.size() == prev_ports) {
            m_pipewire.core->run_once();
        }

        const std::string virtualMicID = std::to_string(m_pipewire.virtualMic.node->id());
        for(auto it = m_pipewire.ports.begin(); it != m_pipewire.ports.end();) {
            auto& port = *it;
            auto props = port.props();

            if(props["node.id"] != virtualMicID) {
                it++;
                continue;
            }

            const auto channel = props["audio.channel"];
            const auto info    = port.info();

            switch(info.direction) {
            case pw::port_direction::input:
                if(channel == "FL") {
                    m_pipewire.virtualMic.inFL = std::move(port);
                    it                         = m_pipewire.ports.erase(it);

                    continue;
                }
                else if(channel == "FR") {
                    m_pipewire.virtualMic.inFR = std::move(port);
                    it                         = m_pipewire.ports.erase(it);

                    continue;
                }

                break;
            case pw::port_direction::output:
                if(channel == "FL") {
                    m_pipewire.virtualMic.outFL = std::move(port);
                    it                          = m_pipewire.ports.erase(it);

                    continue;
                }
                else if(channel == "FR") {
                    m_pipewire.virtualMic.outFR = std::move(port);
                    it                          = m_pipewire.ports.erase(it);

                    continue;
                }

                break;
            default: break;
            }

            it++;
        }

        if(!m_pipewire.virtualMic.inFL || !m_pipewire.virtualMic.inFR || !m_pipewire.virtualMic.outFL || !m_pipewire.virtualMic.outFR) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to get all virtual microphone ports, using SDL3");
            goto cleanup;
        }
    }
    catch(...) {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to initialize pipewire, using SDL3");
        goto cleanup;
    }

    m_usingPipewire = true;

    updatePipewireLink();

    return;
cleanup:
    m_usingPipewire = false;

    try {
        // issue with cleanup unless done in this order
        m_pipewire.virtualMic = {};
        m_pipewire.registry   = std::nullopt;
        m_pipewire.core.reset();
        m_pipewire.context.reset();

        m_pipewire = {};
    }
    catch(...) {
    }
}

void Application::updatePipewireLink() {
    if(!m_usingPipewire || m_pipewire.sinks.empty() || m_pipewire.sources.empty()) {
        return;
    }

    m_pipewire.links.clear();
    m_pipewire.core->run_once();

    std::string sinkName = m_pipewire.sinks[0].name;
    uint32_t sinkNode    = m_pipewire.sinks[0].node.id();

    for(auto& sink : m_pipewire.sinks) {
        if(sink.node.props()["node.name"] == m_pipewire.defaultSinkName) {
            sinkName = sink.name;
            sinkNode = sink.node.id();
        }
    }

    const std::string sourceName = m_settings.getSelectedRecordingDevice();

    m_pipewire.currentSource = &m_pipewire.sources[0];
    for(auto& source : m_pipewire.sources) {
        if(getNodeName(source.node) == sourceName) {
            m_pipewire.currentSource = &source;
            break;
        }
    }

    if(m_pipewire.currentSource == nullptr) {
        return;
    }

    SDL_Log("Using sink: %s", sinkName.c_str());
    SDL_Log("Using source: %s", m_pipewire.currentSource->name.c_str());

    const auto sinkID   = std::to_string(sinkNode);
    const auto sourceID = std::to_string(m_pipewire.currentSource->node.id());

    m_pipewire.lastSinkID = sinkNode;

    for(const pw::port& port : m_pipewire.ports) {
        auto info = port.info();
        if(!info.props.contains("node.id")) {
            continue;
        }

        const auto node    = info.props["node.id"];
        const auto port_id = info.props["port.id"];

        if(port_id != "0" && port_id != "1") {
            continue;
        }

        auto factory = std::optional<pw::link_factory>{};
        if(node == sourceID && info.direction == pipewire::port_direction::output) {
            factory = {
                .input  = (port_id == "0" ? m_pipewire.virtualMic.inFL : m_pipewire.virtualMic.inFR)->id(),
                .output = info.id,
            };
        }

        if(node == sinkID && info.direction == pipewire::port_direction::input) {
            factory = {
                .input  = info.id,
                .output = (port_id == "0" ? m_pipewire.virtualMic.outFL : m_pipewire.virtualMic.outFR)->id(),
            };
        }

        if(!factory) {
            continue;
        }

        auto link = m_pipewire.core->wait(m_pipewire.core->create(factory.value()));
        if(!link) {
            SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to create link (%u -> %u): %s", factory->input, factory->output, link.error().message);
            continue;
        }

        m_pipewire.links.push_back(std::move(link.value()));
    }

    m_pipewire.core->wait(m_pipewire.core->sync());
    _updateVolumePipewire();
}

void Application::setRecordingDevicePipewire(const Application::PipewireDevice& device) {
    m_settings.setSelectedRecordingDevice(device.name);
    updatePipewireLink();

    changeStatus(std::format("Recording Device: {}", device.name), std::chrono::milliseconds(1500));
}

void Application::_updateVolumePipewire() {
    if(!m_usingPipewire) {
        return;
    }

    const float volume = getCubicVolume();
    auto params        = m_pipewire.core->wait(m_pipewire.virtualMic.node->params());

    for(const auto& [pod_id, pod] : params) {
        auto prop = pod.find_recursive(pipewire::spa::prop::channel_volumes);
        if(!prop) {
            continue;
        }

        auto channels      = prop->value().read<std::vector<float>>();
        auto cubic_volumes = channels | std::views::transform([volume](auto&&) { return volume; });

        prop->value().write<std::vector<float>>({ cubic_volumes.begin(), cubic_volumes.end() });

        m_pipewire.virtualMic.node->set_param(pod_id, 0, pod);
        m_pipewire.core->wait(m_pipewire.core->sync());
    }
}

#endif