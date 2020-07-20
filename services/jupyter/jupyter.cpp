#include "jupyter.hpp"

#include <actor-zeta/core.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <components/configuration/configuration.hpp>

#include <nlohmann/json.hpp>

namespace services {

    auto send(zmq::socket_ref socket, std::vector<std::string> msgs) -> void {
        std::vector<zmq::const_buffer> msgs_for_send;

        msgs_for_send.reserve(msgs.size());

        for (const auto& msg : msgs) {
            std::cerr << msg << std::endl;
            msgs_for_send.push_back(zmq::buffer(std::move(msg)));
        }

        auto result = zmq::send_multipart(socket, std::move(msgs_for_send));

        if(!result) {
            throw std::logic_error("Error sending ZeroMQ message");
        }
    }


    namespace nl = nlohmann;

    jupyter::jupyter(
        const components::python_sandbox_configuration& configuration,
        components::log_t& log)
        : mode_{components::sandbox_mode_t::none}
        , zmq_context{nullptr}
        , jupyter_kernel_commands_polls{}
        , jupyter_kernel_infos_polls{}
        , commands_exuctor{nullptr}
        , infos_exuctor{nullptr} {
        log_ = log.clone();
        log_.info("processing env python start ");
        log_.info(fmt::format("Mode : {0}", configuration.mode_));
        mode_ = configuration.mode_;
        if (!configuration.jupyter_connection_path_.empty()) {
            log_.info(fmt::format("jupyter connection path : {0}", configuration.jupyter_connection_path_.string()));
        }
        jupyter_connection_path_ = configuration.jupyter_connection_path_;
        log_.info("processing env python finish ");
        add_handler("write",&jupyter::write);
    }

    auto jupyter::jupyter_kernel_init() -> void {
        std::ifstream connection_file{jupyter_connection_path_.string()};

        if (!connection_file) {
            throw std::logic_error("File jupyter_connection not found");
        }

        nl::json configuration;

        connection_file >> configuration;

        std::cerr << configuration.dump(4) << std::endl;

        std::string transport{configuration["transport"]};
        std::string ip{configuration["ip"]};
        auto shell_port{std::to_string(configuration["shell_port"]
                                           .get<std::uint16_t>())};
        auto control_port{std::to_string(configuration["control_port"]
                                             .get<std::uint16_t>())};
        auto stdin_port{std::to_string(configuration["stdin_port"]
                                           .get<std::uint16_t>())};
        auto iopub_port{std::to_string(configuration["iopub_port"]
                                           .get<std::uint16_t>())};
        auto heartbeat_port{std::to_string(configuration["hb_port"]
                                               .get<std::uint16_t>())};
        auto shell_address{transport + "://" + ip + ":" + shell_port};
        auto control_address{transport + "://" + ip + ":" + control_port};
        auto stdin_address{transport + "://" + ip + ":" + stdin_port};
        auto iopub_address{transport + "://" + ip + ":" + iopub_port};
        auto heartbeat_address{transport + "://" + ip + ":" + heartbeat_port};

        zmq_context = std::make_unique<zmq::context_t>();
        zmq::socket_t shell_socket{*zmq_context, zmq::socket_type::router};
        zmq::socket_t control_socket{*zmq_context, zmq::socket_type::router};
        zmq::socket_t stdin_socket{*zmq_context, zmq::socket_type::router};
        zmq::socket_t iopub_socket{*zmq_context, zmq::socket_type::pub};
        zmq::socket_t heartbeat_socket{*zmq_context, zmq::socket_type::rep};

        shell_socket.setsockopt(ZMQ_LINGER, 1000);
        control_socket.setsockopt(ZMQ_LINGER, 1000);
        stdin_socket.setsockopt(ZMQ_LINGER, 1000);
        iopub_socket.setsockopt(ZMQ_LINGER, 1000);
        heartbeat_socket.setsockopt(ZMQ_LINGER, 1000);

        shell_socket.bind(shell_address);
        control_socket.bind(control_address);
        stdin_socket.bind(stdin_address);
        iopub_socket.bind(iopub_address);
        heartbeat_socket.bind(heartbeat_address);

        jupyter_kernel_commands_polls = {{shell_socket, 0, ZMQ_POLLIN, 0},
                                         {control_socket, 0, ZMQ_POLLIN, 0}};
        jupyter_kernel_infos_polls = {{heartbeat_socket, 0, ZMQ_POLLIN, 0}};
        engine_mode = false;

    }

    auto jupyter::jupyter_engine_init() -> void {
        std::ifstream connection_file{jupyter_connection_path_.string()};

        if (!connection_file) {
            throw std::logic_error("File jupyter_connection not found");
        }

        nl::json configuration;

        connection_file >> configuration;

        std::cerr << configuration.dump(4) << std::endl;

        std::string interface{configuration["interface"]};
        auto mux_port{std::to_string(configuration["mux"]
                                         .get<std::uint16_t>())};
        auto task_port{std::to_string(configuration["task"]
                                          .get<std::uint16_t>())};
        auto control_port{std::to_string(configuration["control"]
                                             .get<std::uint16_t>())};
        auto iopub_port{std::to_string(configuration["iopub"]
                                           .get<std::uint16_t>())};
        auto heartbeat_ping_port{std::to_string(configuration["hb_ping"]
                                                    .get<std::uint16_t>())};
        auto heartbeat_pong_port{std::to_string(configuration["hb_pong"]
                                                    .get<std::uint16_t>())};
        auto registration_port{std::to_string(configuration["registration"]
                                                  .get<std::uint16_t>())};
        auto mux_address{interface + ":" + mux_port};
        auto task_address{interface + ":" + task_port};
        auto control_address{interface + ":" + control_port};
        auto iopub_address{interface + ":" + iopub_port};
        auto heartbeat_ping_address{interface + ":" + heartbeat_ping_port};
        auto heartbeat_pong_address{interface + ":" + heartbeat_pong_port};
        auto registration_address{interface + ":" + registration_port};

        zmq_context = std::make_unique<zmq::context_t>();
        zmq::socket_t shell_socket{*zmq_context, zmq::socket_type::router};
        zmq::socket_t control_socket{*zmq_context, zmq::socket_type::router};
        zmq::socket_t iopub_socket{*zmq_context, zmq::socket_type::pub};

        heartbeat_ping_socket = zmq::socket_t{*zmq_context,
                                              zmq::socket_type::sub};
        heartbeat_pong_socket = zmq::socket_t{*zmq_context,
                                              zmq::socket_type::dealer};

        zmq::socket_t registration_socket{*zmq_context,
                                          zmq::socket_type::dealer};

        auto identifier{boost::uuids::random_generator()()};
        auto identifier_raw{boost::uuids::to_string(identifier)};

        shell_socket.setsockopt(ZMQ_ROUTING_ID, identifier_raw.c_str(),
                                identifier_raw.size());
        control_socket.setsockopt(ZMQ_ROUTING_ID, identifier_raw.c_str(),
                                  identifier_raw.size());
        iopub_socket.setsockopt(ZMQ_ROUTING_ID, identifier_raw.c_str(),
                                identifier_raw.size());
        heartbeat_ping_socket.setsockopt(ZMQ_SUBSCRIBE, "", 0);
        heartbeat_pong_socket.setsockopt(ZMQ_ROUTING_ID,
                                         identifier_raw.c_str(),
                                         identifier_raw.size());

        shell_socket.connect(mux_address);
        shell_socket.connect(task_address);
        control_socket.connect(control_address);
        iopub_socket.connect(iopub_address);
        heartbeat_ping_socket.connect(heartbeat_ping_address);
        heartbeat_pong_socket.connect(heartbeat_pong_address);
        registration_socket.connect(registration_address);

        jupyter_kernel_commands_polls = {{shell_socket, 0, ZMQ_POLLIN, 0},
                                         {control_socket, 0, ZMQ_POLLIN, 0}};
        engine_mode=true;

    }

    void heartbeat(bool engine_mode,zmq::socket_t& heartbeat_socket) {
        if (!engine_mode) {
            std::vector<zmq::message_t> msgs;

            while (zmq::recv_multipart(heartbeat_socket,
                                       std::back_inserter(msgs),
                                       zmq::recv_flags::dontwait)) {
                for (const auto& msg : msgs) {
                    std::cerr << "Heartbeat: " << msg << std::endl;
                }

                zmq::send_multipart(heartbeat_socket,std::move(msgs),zmq::send_flags::dontwait);
            }
        }
    }

    auto jupyter::start() -> void {
         if (mode_ == components::sandbox_mode_t::jupyter_kernel ||
                   mode_ == components::sandbox_mode_t::jupyter_engine) {
            commands_exuctor = std::make_unique<std::thread>([this]() {
              if (mode_ == components::sandbox_mode_t::jupyter_engine) {
                  auto tmp = jupyter_kernel->registration();
                  send(*registration_socket,tmp);
                  std::vector<zmq::message_t> msgs;

                  if (!zmq::recv_multipart(*registration_socket,
                                           std::back_inserter(msgs))) {
                      return false;
                  }

                  std::vector<std::string> msgs_for_parse;

                  msgs_for_parse.reserve(msgs.size());

                  for (const auto& msg : msgs) {
                      std::cerr << "Registration: " << msg << std::endl;
                      msgs_for_parse.push_back(std::move(msg.to_string()));
                  }

                  jupyter_kernel->registration(std::move(msgs_for_parse)) ;

              }
              bool e;
              while (e) {
                  if (zmq::poll(jupyter_kernel_commands_polls) == -1) {
                      continue;
                  }

                  std::vector<zmq::message_t> msgs;

                  if (jupyter_kernel_commands_polls[0].revents & ZMQ_POLLIN) {
                      while (zmq::recv_multipart(shell_socket,
                                                 std::back_inserter(msgs),
                                                 zmq::recv_flags::dontwait)) {
                          std::vector<std::string> msgs_for_parse;

                          msgs_for_parse.reserve(msgs.size());

                          for (const auto& msg : msgs) {
                              std::cerr << "Shell: " << msg << std::endl;
                              msgs_for_parse.push_back(std::move(msg.to_string()));
                          }

                      }
                  }

                  if (jupyter_kernel_commands_polls[1].revents & ZMQ_POLLIN) {
                      while (zmq::recv_multipart(control_socket,
                                                 std::back_inserter(msgs),
                                                 zmq::recv_flags::dontwait)) {
                          std::vector<std::string> msgs_for_parse;

                          msgs_for_parse.reserve(msgs.size());

                          for (const auto& msg : msgs) {
                              std::cerr << "Control: " << msg << std::endl;
                              msgs_for_parse.push_back(std::move(msg.to_string()));
                          }

                      }
                  }


              }
            });

            infos_exuctor = std::make_unique<std::thread>([this]() {
              if (mode_ == components::sandbox_mode_t::jupyter_kernel) {
                  bool e;
                  while (e) {
                      if (zmq::poll(jupyter_kernel_infos_polls) == -1) {
                          continue;
                      }

                      heartbeat(engine_mode,*heartbeat_socket);

                  }
              } else {
                  zmq::proxy(heartbeat_ping_socket, heartbeat_pong_socket);
              }
            });
        }
    }

    auto jupyter::init() -> void {

        if (components::sandbox_mode_t::jupyter_kernel == mode_) {
            log_.info("jupyter kernel mode");
            jupyter_kernel_init();
        } else if (components::sandbox_mode_t::jupyter_engine == mode_) {
            log_.info("jupyter engine mode");
            jupyter_engine_init();
        } else {
            log_.info("init script mode ");
        }
    }

    jupyter::~jupyter() = default;

    void jupyter::enqueue(goblin_engineer::message, actor_zeta::executor::execution_device*) {
    }
    auto jupyter::write(const std::string&socket_type,std::vector<std::string>&msg) -> void {

        if("iopub" == socket_type){
            send(iopub_socket,msg);
        }

        if("shell" == socket_type){
            send(shell_socket,msg);
        }

        if("control" == socket_type){
            send(control_socket,msg);
        }

    }

} // namespace services