#pragma once

#include <json.hpp>
#include <vector>

namespace next_stop {

    using json = nlohmann::json;
    using timestamp_t = int64_t;

    timestamp_t parse_timestamp(const std::string& timestamp) {
        return std::stoll(timestamp);
    }

    // template <class Input, typename... Types>
    // void parse(Input&& text, const Types&& ...keys) {
    //     auto data = json::parse(text);
    //     for (auto it : data) {
    //         auto s = it["direction"].template get<std::string>();
    //         auto arriving_at = it["arriving_at_timestamp"].template get<int64_t>();
    //         std::cout << " Parsed " << s << " " << arriving_at << "\n";
    //     }
    // }

    template <class T>
    struct argument {
        const char* key;
        argument(const char* _key) : key(_key) {}
    };

    template <class J, class... Ts>
    std::tuple<Ts...> extract_key(J&& obj, const argument<Ts>&&... args) {
        return std::make_tuple(
            std::move(obj[args.key].template get<Ts>())...
        );
    }

    template <class Input>
    void parse(Input&& text) {
        auto data = json::parse(text);
        for (auto it : data) {
            auto tup1 = extract_key(
                std::move(it), 
                argument<std::string>("direction"),
                argument<int64_t>("arriving_at_timestamp")
            );
        }
    }

    template <class Input, class... Ts>
    std::vector<std::tuple<Ts...>> parse(Input&& text, const argument<Ts>&&... args) {
        auto result = std::vector<std::tuple<Ts...>>();
        auto data = json::parse(text);
        for (auto it : data) {
            result.push_back(extract_key(
                std::move(it), 
                std::move(args)...
            ));
        }
        return result;
    }

    template <class Input>
    std::vector<int> next_minutes(const std::string& timestamp, Input&& body) {
        // TODO: If timestamp is null or empty
        timestamp_t sample_ts = parse_timestamp(timestamp);

        auto parsed = parse(std::move(body), argument<int64_t>("arriving_at_timestamp"));

        auto results = std::vector<int>(parsed.size());
        std::transform(parsed.begin(), parsed.end(), results.begin(), [&sample_ts](const decltype(parsed[0])& tup) {
            return (std::get<0>(tup) - sample_ts) / 60;
        });
        return results;
    }
}
