#include <iostream>
#include <App.h>
#include <thread>
#include <algorithm>
#include <atomic>
using namespace std;

struct UserConnection {
    unsigned long user_id; // очень большие беззнаковые числа
    string user_name;
};

int main()
{
    // ws://127.0.0.1:9999/

    atomic_ulong latest_user_id = 10;
    vector<thread*> threads(thread::hardware_concurrency());

    transform(threads.begin(), threads.end(), threads.begin(), [&latest_user_id](thread* thr) {
        return new thread([&latest_user_id]() {
            // Что делает данный поток
            uWS::App().ws<UserConnection>("/*", {
                // Что делать серверу в разных ситуациях
                .open = [&latest_user_id](auto* ws) {
                    // Код, который выполняется при новом подключении
                    UserConnection* data = (UserConnection*)ws->getUserData();
                    data->user_id = latest_user_id++;
                    cout << "New user connected, id = " << data->user_id << endl;
                    ws->subscribe("broadcast");
                    ws->subscribe("user#" + to_string(data->user_id));
                },

                .message = [](auto* ws, string_view message, uWS::OpCode opCode) {
                    // Код, который выполняется при получении нового сообщения
                    UserConnection* data = (UserConnection*)ws->getUserData();
                    // SET_NAME=Mike
                    string_view beginning = message.substr(0,9);
                    if (beginning.compare("SET_NAME=") == 0) {
                        // Пользователь чата представляется
                        string_view name = message.substr(9);
                        data->user_name = string(name);
                        cout << "User set their name ( ID= " << data->user_id << " ), name = " << data->user_name << endl;
                        // NEW_USER,Mike,10
                        // Уведомляем остальных пользователей чата о новом участнике
                        string broadcast_message = "NEW_USER," + data->user_name + "," + to_string(data->user_id);
                        ws->publish("broadcast", string_view(broadcast_message), opCode, false);
                    }

                    // MESSAGE_TO,11,Hello how are you???
                    string_view is_message_to = message.substr(0, 11);
                    if (is_message_to.compare("MESSAGE_TO,") == 0) {
                        string_view rest = message.substr(11); // id,message
                        int comma_position = rest.find(",");
                        string_view id_string = rest.substr(0, comma_position); // id
                        string_view user_message = rest.substr(comma_position + 1); // message
                        ws->publish("user#" + string(id_string), user_message, opCode, false);
                        cout << "New private message " << message << endl;
                    }
                }

            })
            .listen(9001, [](auto* token) {
                if (token) {
                    cout << "Server successfully started on port 9001\n";
                }
                else {
                    cout << "Something went wrong\n";
                }
            })
            .run();
        });
    });

    for_each(threads.begin(), threads.end(), [](thread* thr) {
        thr->join();
    });
}
