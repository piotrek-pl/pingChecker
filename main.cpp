#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <windows.h>
#include <thread>
#include <chrono>
#include <iomanip>

// Funkcja pomocnicza zwracająca wartość klucza jako int
int handleIntKey(const boost::property_tree::ptree& pt,
                 const std::string& key,
                 int defaultValue)
{
    boost::optional<std::string> valueOpt = pt.get_optional<std::string>(key);

    if (valueOpt && !valueOpt->empty())
    {
        try
        {
            int value = std::stoi(valueOpt.get());
            std::cout << "Wartość klucza '" << key << "': " << value << std::endl;
                return value;
        }
        catch (const std::exception& e)
        {
            std::cout << "Nieprawidłowa wartość dla klucza '" << key << "'. Zastosowano wartość domyślną: " << defaultValue << std::endl;
        }
    }
    else
    {
        std::cout << "Brak wartości dla klucza '" << key << "'. Zastosowano wartość domyślną: " << defaultValue << std::endl;
    }

    return defaultValue;
}

// Funkcja pomocnicza zwracająca wartość klucza jako string
std::string handleStringKey(const boost::property_tree::ptree& pt,
                            const std::string& key,
                            const std::string& defaultValue)
{
    boost::optional<std::string> valueOpt = pt.get_optional<std::string>(key);

    if (valueOpt)
    {
        std::string value = valueOpt.get();
        std::cout << "Wartość klucza '" << key << "': " << value << std::endl;
            return value;
    }
    else
    {
        std::cout << "Brak wartości dla klucza '" << key << "'. Zastosowano wartość domyślną: " << defaultValue << std::endl;
            return defaultValue;
    }
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);
    boost::property_tree::ptree pt;

    int pingFrequency, responseThreshold;
    std::string logFilePath, target;

    try
    {
        boost::property_tree::ini_parser::read_ini("config.ini", pt);

        // Przypisanie wartości kluczy do zmiennych
        pingFrequency = handleIntKey(pt, "Settings.PingFrequency", 10);
        responseThreshold = handleIntKey(pt, "Settings.ResponseThreshold", 100);
        logFilePath = handleStringKey(pt, "Settings.LogFilePath", "ping_log.txt");
        target = handleStringKey(pt, "Settings.Target", "www.google.com");
    }
    catch (const boost::property_tree::ini_parser_error &e)
    {
        std::cout << "Błąd odczytu pliku INI: " << e.what() << std::endl;
        // Ustawienie wartości domyślnych
        pingFrequency = 1;
        responseThreshold = 1;
        logFilePath = "ping_log.txt";
        target = "www.google.com";
    }

    while (true)
    {
        // Wywołanie polecenia ping i przechwycenie wyjścia
        std::string command = "ping " + target + " -n 1";
        std::array<char, 256> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
        if (!pipe)
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }

        // Wyświetlenie wyniku ping
        std::cout << result;

        // Analiza wyniku
        bool hostUnreachable = result.find("Destination host unreachable") != std::string::npos ||
                               result.find("Request timed out") != std::string::npos;
        bool generalFailure = result.find("General failure") != std::string::npos;
        bool hostNotFound = result.find("could not find host") != std::string::npos;


        std::size_t timePos = result.find("time=");
        if (timePos != std::string::npos)
        {
            std::size_t start = timePos + 5; // 5 to długość "time="
            std::size_t end = result.find("ms", start);
            if (end != std::string::npos)
            {
                std::string responseTimeStr = result.substr(start, end - start);
                try
                {
                    int responseTime = std::stoi(responseTimeStr);
                    if (responseTime > responseThreshold)
                    {
                        // Pobranie aktualnego czasu
                        auto now = std::chrono::system_clock::now();
                        auto now_c = std::chrono::system_clock::to_time_t(now);

                        // Zapisanie do pliku
                        std::ofstream file(logFilePath, std::ios::app);
                        if (file.is_open())
                        {
                            file << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S")
                                 << " - Ping to " << target << ": " << responseTime << " ms" << std::endl;
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error parsing response time: " << e.what() << std::endl;
                }
            }
        }
        else if (hostUnreachable || generalFailure || hostNotFound)
        {
            // Pobranie aktualnego czasu
            auto now = std::chrono::system_clock::now();
            auto now_c = std::chrono::system_clock::to_time_t(now);

            // Zapisanie informacji o błędzie do pliku
            std::ofstream file(logFilePath, std::ios::app);
            if (file.is_open())
            {
                file << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S")
                     << " - Error: ";
                if (hostUnreachable) {
                    file << "Host " << target << " is unreachable.";
                } else if (generalFailure) {
                    file << "General failure occurred.";
                } else if (hostNotFound) {
                    file << "Could not find host " << target << ".";
                }
                file << std::endl;
            }
        }

        // Czekanie na następny ping
        std::this_thread::sleep_for(std::chrono::seconds(pingFrequency));
    }


    system("pause");
    return 0;
}
