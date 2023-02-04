#include <gtest/gtest.h>
#include <Config.hpp>

using namespace config;

const auto s = R"({
    "active_stop": 1,
    "connection": {
        "ssid":  "ssid",
        "password": "password"
    },
    "stops": [{
        "name": "1 Train South",
        "direction": "South", 
        "url": "http://url1"
    }, {
        "name": "1 Train North",
        "direction": "North", 
        "url": "http://url2"
    }]
}
)";

TEST(ParseConfigReturnsTrue, ShouldPass) {
    Config result;
    parse(s, &result);

    EXPECT_EQ(
        parse(s, &result), true
    );
}

TEST(ParseConfig, ShouldPass) {
    Config result;
    auto success = parse(s, &result);

    EXPECT_EQ(success, true);
    EXPECT_EQ(result.active_stop, 1);

    EXPECT_EQ(result.stops.size(), 2);
    std::vector<std::string> urls(2);
    std::transform(result.stops.cbegin(), result.stops.cend(), urls.begin(), [](const Stop& s) { return s.url; });
    EXPECT_EQ(urls, std::vector<std::string>({"http://url1", "http://url2"}));

    EXPECT_EQ(result.connection.ssid, "ssid");
    EXPECT_EQ(result.connection.password, "password");
    
}
