/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin-protocol.
 *
 * libbitcoin-protocol is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <string>
#include <thread>
#include <bitcoin/protocol.hpp>

using namespace bc;
using namespace bc::protocol;

void server_task(const std::string& server_private_key,
    const std::string& client_public_key,
    const config::authority& client_address)
{
    // Create a context for the server.
    zmq::context context;
    assert(context);

    // Establish the context's authentication whitelist.
    zmq::authenticator authenticator(context);
    authenticator.allow(client_address);
    authenticator.allow(client_public_key);

    // Create a push socket using the server's authenticated context.
    zmq::socket server(context, zmq::socket::role::pusher);
    assert(server);

    // Configure the server to provide identity and require client identity.
    auto result = server.set_private_key(server_private_key);
    assert(result);
    result = server.set_curve_server();
    assert(result);

    // Bind the server to a tcp port on all local addresses.
    result = server.bind("tcp://*:9000");
    assert(result);

    //  Send the test message.
    zmq::message message;
    message.append("helllo world!");
    result = message.send(server);
    assert(result);

    // Give client time to complete (hack).
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
}

void client_task(const std::string& client_private_key,
    const std::string& server_public_key)
{
    // Create a context for the client.
    zmq::context context;
    assert(context);

    // Bind a pull socket to the client context.
    zmq::socket client(context, zmq::socket::role::puller);
    assert(client);

    // Configure the client to provide identity and require server identity.
    auto result = client.set_private_key(client_private_key);
    assert(result);
    result = client.set_curve_client(server_public_key);
    assert(result);

    // Connect to the server's tcp port on the local host.
    result = client.connect("tcp://127.0.0.1:9000");
    assert(result);

    // Wait for the message, which signals the test was successful.
    zmq::message message;
    result = message.receive(client);
    assert(result);
    assert(message.text() == "helllo world!");

    puts("Ironhouse test OK");
}

int ironhouse2_example()
{
    static const auto localhost = config::authority("127.0.0.1");

    // Create client and server certificates (generated secrets).
    zmq::certificate client_cert;
    assert(client_cert);
    zmq::certificate server_cert;
    assert(server_cert);

    // Start a server, require the client cert and localhost address.
    std::thread server_thread(server_task, server_cert.private_key(),
        client_cert.public_key(), localhost);

    // Start a client, allos connections only to server with cert.
    std::thread client_thread(client_task, client_cert.private_key(),
        server_cert.public_key());

    // Wait for thread completions.
    client_thread.join();
    server_thread.join();
    return 0;
}
