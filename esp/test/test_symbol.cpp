#include <gtest/gtest.h>
#include <symbol.hpp>

// TEST(...)
// TEST_F(...)

template <class Symb, class Iter>
int getCols(symbol::ISymbol<Symb, Iter>& s) {
    return s.cols();
}

TEST(DummyTest, ShouldPass) {
    auto symb = symbol::ZERO;
    
    EXPECT_EQ(symb.size(), 8 * 5);
    EXPECT_EQ(symb.cols(), 5);
    EXPECT_EQ(getCols(symb), 5);

    auto iter = symb.rows();
    auto b = iter.begin();
    auto e = iter.end();
    
    EXPECT_EQ(e._sz, 0);

    EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, false);

    ++b; EXPECT_EQ(b==e, true);
}

TEST(IteratorTest, ShouldPass) {
    int col = 0;
    for (auto row : symbol::ZERO.rows_iter()) {
        for (int i = 0; i < 8; i++) {
            auto d = row[(col % 2) == 0 ? i : 7 - i];
        }
        col++;
    }

    EXPECT_EQ(col, symbol::ZERO.cols());
}

TEST(SymArrayIteratorBuildTest, ShouldPass) {
    auto symbols = symbol::SymbolArray( {
        symbol::ZERO,
        symbol::ZERO,
        symbol::ZERO
    });

    auto iter = symbols.rows_iter();
}

TEST(SymArrayIteratorTest_Sanity, ShouldPass) {
    auto symbols = symbol::SymbolArray( {
        symbol::ZERO,
    });

    auto iter = symbols.rows_iter();
    auto b = iter.begin();
    auto e = iter.end();

    EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, false);
    ++b; EXPECT_EQ(b==e, true);
}

TEST(SymArrayIteratorTest_Sanity2, ShouldPass) {
    const auto symbols = symbol::SymbolArray( {
        symbol::ZERO,
        symbol::ZERO,
    });

    auto iter = symbols.rows_iter();
    auto b = iter.begin();
    auto e = iter.end();

    
    for (int c = 0; c < symbols.cols(); c++) {
        EXPECT_EQ(b==e, false);
        ++b;
    }

    EXPECT_EQ(b==e, true);
}

TEST(SymArrayIteratorRangeTest, ShouldPass) {
    const auto symbols = symbol::SymbolArray( {
        symbol::ZERO,
        symbol::ZERO,
        symbol::SPACE
    });

    int c = 0;
    for (auto row : symbols.rows_iter()) {
         c++;
    }
    EXPECT_EQ(c, symbol::ZERO.cols() + symbol::ZERO.cols() + symbol::SPACE.cols());
    EXPECT_EQ(c, symbols.cols());
}


template <class Sym, class Iter>
int iterSym(const symbol::ISymbol<Sym, Iter>& symbol) {
    int col = 0;
    for (auto row : symbol.rows_iter()) {
        // auto c = col - offset;
        // if (c >= start && c < end) drawColumn(c, row, color);
        col++;
    }
    return col;
}

TEST(SymArrayIteratorRangeTemplateTest, ShouldPass) {
    const auto symbols = symbol::SymbolArray( {
        symbol::ZERO,
        symbol::ZERO,
        symbol::SPACE
    });

    int c = iterSym(symbols);
    EXPECT_EQ(c, symbol::ZERO.cols() + symbol::ZERO.cols() + symbol::SPACE.cols());
    EXPECT_EQ(c, symbols.cols());
}

TEST(SymArrayMutationTest, ShouldPass) {
    auto symbols = symbol::SymbolArray( {
    });

    symbols.append(symbol::ZERO);
    EXPECT_EQ(iterSym(symbols), symbol::ZERO.cols());

    symbols.append(symbol::ZERO);
    EXPECT_EQ(iterSym(symbols), symbol::ZERO.cols() + symbol::ZERO.cols());
}
