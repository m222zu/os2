//2-3만 완료
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <map>
#include <chrono>
#include <mutex>
#include <cmath>
#include <future>

using namespace std;

mutex cout_mutex;
int bg_count = 0;

vector<string> split(const string& str, char delim) {
    stringstream ss(str);
    string token;
    vector<string> tokens;
    while (getline(ss, token, delim)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int prime(int n) {
    if (n < 2) return 0;
    vector<bool> is_prime(n + 1, true);
    is_prime[0] = is_prime[1] = false;
    for (int i = 2; i <= sqrt(n); ++i) {
        if (is_prime[i]) {
            for (int j = i * i; j <= n; j += i) {
                is_prime[j] = false;
            }
        }
    }
    return count(is_prime.begin(), is_prime.end(), true);
}

int sum_part(int start, int end) {
    int result = 0;
    for (int i = start; i <= end; ++i) {
        result = (result + i) % 1000000;
    }
    return result;
}

int sum(int n, int m) {
    vector<future<int>> futures;
    int part_size = n / m;
    for (int i = 0; i < m; ++i) {
        int start = i * part_size + 1;
        int end = (i == m - 1) ? n : (i + 1) * part_size;
        futures.push_back(async(launch::async, sum_part, start, end));
    }
    int result = 0;
    for (auto& fut : futures) {
        result = (result + fut.get()) % 1000000;
    }
    return result;
}

void execute_command(const vector<string>& args, bool is_bg = false, int duration = 0) {
    auto start_time = chrono::steady_clock::now();
    while (true) {
        if (!args.empty() && args[0] == "echo") {
            unique_lock<mutex> lock(cout_mutex);
            if (args.size() > 1) cout << args[1] << endl;
        }
        else if (args[0] == "dummy") {
        }
        else if (args[0] == "gcd") {
            if (args.size() > 2) {
                int a = stoi(args[1]);
                int b = stoi(args[2]);
                unique_lock<mutex> lock(cout_mutex);
                cout << gcd(a, b) << endl;
            }
        }
        else if (args[0] == "prime") {
            if (args.size() > 1) {
                int n = stoi(args[1]);
                unique_lock<mutex> lock(cout_mutex);
                cout << prime(n) << endl;
            }
        }
        else if (args[0] == "sum") {
            if (args.size() > 1) {
                int n = stoi(args[1]);
                int m = 1;
                for (size_t i = 2; i < args.size(); ++i) {
                    if (args[i] == "-m" && i + 1 < args.size()) {
                        m = stoi(args[++i]);
                    }
                }
                unique_lock<mutex> lock(cout_mutex);
                cout << sum(n, m) << endl;
            }
        }
        if (is_bg) {
            unique_lock<mutex> lock(cout_mutex);
            bg_count--;
        }
        if (duration > 0) {
            auto current_time = chrono::steady_clock::now();
            auto elapsed = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
            if (elapsed >= duration) break;
        }
        break;
    }
}

void thread_function(vector<string> args, int repeat, int period, int duration) {
    for (int i = 0; i < repeat; ++i) {
        execute_command(args, true, duration);
        this_thread::sleep_for(chrono::seconds(period));
    }
}

vector<string> parse(const string& command) {
    return split(command, ' ');
}

void exec(vector<string> args, bool is_bg = false, int duration = 0) {
    if (args.empty()) return;
    execute_command(args, is_bg, duration);
}

void processCommands(const string& filename, int interval) {
    ifstream file(filename);
    string line;

    while (getline(file, line)) {
        cout << "prompt> " << line << endl;
        vector<string> commands = split(line, ';');
        vector<thread> bg_threads;

        for (const string& cmd : commands) {
            vector<string> args = parse(cmd);
            bool is_bg = false;
            int repeat = 1;
            int period = 0;
            int duration = 0;

            if (!args.empty() && args[0].front() == '&') {
                args[0] = args[0].substr(1);
                is_bg = true;
                bg_count++;
            }

            for (size_t i = 0; i < args.size(); ++i) {
                if (args[i] == "-n" && i + 1 < args.size()) {
                    repeat = stoi(args[++i]);
                }
                else if (args[i] == "-p" && i + 1 < args.size()) {
                    period = stoi(args[++i]);
                }
                else if (args[i] == "-d" && i + 1 < args.size()) {
                    duration = stoi(args[++i]);
                }
            }

            if (is_bg) {
                bg_threads.emplace_back(thread_function, args, repeat, period, duration);
            }
            else {
                for (int i = 0; i < repeat; ++i) {
                    exec(args, false, duration);
                    if (period > 0) {
                        this_thread::sleep_for(chrono::seconds(period));
                    }
                }
            }
        }

        for (auto& th : bg_threads) {
            if (th.joinable()) {
                th.detach();
            }
        }

        this_thread::sleep_for(chrono::seconds(interval));
    }
}

int main() {
    string filename = "commands.txt";
    int interval = 5;

    processCommands(filename, interval);
    this_thread::sleep_for(chrono::seconds(interval * 10));

    return 0;
}
