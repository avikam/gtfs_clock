#include <gtest/gtest.h>
#include <NextStopClient.hpp>

using namespace next_stop;

const auto s = R"([
    {"stop": "116 St-Columbia University","line": "1",
    "direction": "117N","leaving_at_timestamp": 1674701220,
    "arriving_at_timestamp": 1674701221}
])";

const auto longer = R"([{"stop":"116 St-Columbia University","line":"1","direction":"117N","leaving_at_timestamp":1674785619,"arriving_at_timestamp":1674785619},{"stop":"116 St-Columbia University","line":"1","direction":"117S","leaving_at_timestamp":1674785674,"arriving_at_timestamp":1674785674},{"stop":"116 St-Columbia University","line":"1","direction":"117N","leaving_at_timestamp":1674785753,"arriving_at_timestamp":1674785753},{"stop":"116 St-Columbia University","line":"1","direction":"117S","leaving_at_timestamp":1674785891,"arriving_at_timestamp":1674785891},{"stop":"116 St-Columbia University","line":"1","direction":"117N","leaving_at_timestamp":1674786007,"arriving_at_timestamp":1674786007},{"stop":"116 St-Columbia University","line":"1","direction":"117S","leaving_at_timestamp":1674786125,"arriving_at_timestamp":1674786125}])";

TEST(ParseTimestampTest, ShouldPass) {
    EXPECT_EQ(
        parse_timestamp("1674698100"), 1674698100
    );
}

TEST(ParseResponse, ShouldPass) {
    {
        auto result = parse(s, argument<std::string>("direction"),
            argument<int64_t>("arriving_at_timestamp")
        );

        EXPECT_EQ(result.size(), 1);
        EXPECT_EQ(std::get<0>(result[0]), "117N");
        EXPECT_EQ(std::get<1>(result[0]), 1674701221);
    }

    {
        auto result = parse(s, argument<std::string>("direction"),
            argument<int64_t>("arriving_at_timestamp")
        );

        EXPECT_EQ(result.size(), 1);

        // It worked but ide complains and compiler warns :/
        // auto [arriving, direction] = result[0]; // c++17
        int64_t arriving;
        std::string direction;
        std::tie(direction, arriving) = result[0];

        EXPECT_EQ(arriving, 1674701221);
        EXPECT_EQ(direction, "117N");
    }
}


TEST(NextStopClientTest, ShouldPass) {
    EXPECT_EQ(
        next_minutes("1674698100", s),
        std::vector<int>({(1674701221 - 1674698100) / 60})
    );

    for (auto x : next_minutes("1674785282", longer)) {
        std::cout << x << "\n";
    }
}
