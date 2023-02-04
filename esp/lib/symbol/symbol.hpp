#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>
#include <initializer_list>
#include <algorithm>

namespace symbol {

class RowsIter {
    public: 
    const char* _p;
    std::size_t _sz;
    RowsIter() : _p(0), _sz(0) {}
    RowsIter(const char* p, std::size_t sz) : _p(p), _sz(sz) {}

    const RowsIter& begin() const { return *this; }
    const RowsIter end() const { return RowsIter(_p + _sz, 0); }

    inline const RowsIter& operator++ ()  {
        _p += 8; _sz -= 8;
        return *this;
    }

    const char* operator*() const {
        return _p;
    }

    inline bool operator== (const RowsIter& rhs) { return (_p == rhs._p); }
    inline bool operator!= (const RowsIter& rhs) { return !(*this == rhs); }
};

template<class Symb, class Iter>
class ISymbol {
    public:
    std::size_t cols() const { 
      return static_cast<const Symb*>(this)->cols();
    }

    Iter rows_iter() const {
      return static_cast<const Symb*>(this)->rows_iter();
    }
};

class constSym8 : public ISymbol<constSym8, RowsIter> {
    const char* p;
    std::size_t sz;

    public:
        using It = RowsIter;

        template<std::size_t N>
        constexpr constSym8(const char(&a)[N]): p(a), sz(N) {}

        constexpr std::size_t size() const { return sz; }
        constexpr std::size_t cols() const { return sz / 8; }

        RowsIter rows() const {
            return RowsIter(p, sz);
        }

        RowsIter rows_iter() const {
            return RowsIter(p, sz);
        }
};

class ChainedIter {
    using inner_type = constSym8::It;

    std::vector<inner_type> _iterators;

    typename std::vector<inner_type>::const_iterator outer;
    typename std::vector<inner_type>::const_iterator outer_end;
    inner_type inner;


    ChainedIter(typename std::vector<inner_type>::const_iterator outer, inner_type inner) : outer(outer), outer_end(outer), inner(inner) {};

    public: 
    ChainedIter(std::vector<inner_type>&& iterators) :
    _iterators(std::move(iterators)), 
    outer(_iterators.cbegin()),
    outer_end(_iterators.cend())
     {
        if (outer != _iterators.end()) {
            inner = outer->begin();
        }
    }

    const ChainedIter& begin() const { return *this; }
    const ChainedIter end() const { 
        return ChainedIter(_iterators.end(), inner_type()); 
    }

    inline const ChainedIter& operator++ ()  {
        // std::cout << "inc inner\n" ;
        ++inner;
        
        if (inner == outer->end()) {
            // std::cout << "inc outer\n" ;
            ++outer;
            if (outer != outer_end) inner = outer->begin();
            else {
                // std::cout << "outer ended\n" ;
                inner = inner_type();
            }
        }
        return *this;
    }

    inline const char* operator*() const {
        return *inner;
    }

    inline bool operator== (const ChainedIter& rhs) { return (outer == rhs.outer && inner == rhs.inner); }
    inline bool operator!= (const ChainedIter& rhs) { return !(*this == rhs); }
};

class SymbolArray : public ISymbol<SymbolArray, ChainedIter> {
    // TODO: This class mains copies of symbols.
    // Since all symbols are static in this code, it could potentially just hold const pointers. 

    std::vector<constSym8> _symbols; // one downside of static interface, is the container is limited to a single type of ISymbol<T>
    std::size_t _cols;
    
    void setCols() {
        _cols = 0;
        for(const auto& s : _symbols) {
            _cols += s.cols();
        }
    }

    public:
    SymbolArray(std::initializer_list<constSym8> init) : _symbols(init) { setCols(); }
    SymbolArray() : SymbolArray({}) {};

    std::size_t cols() const { return _cols; }

    ChainedIter rows_iter() const {
        std::vector<constSym8::It> result(_symbols.size());
        std::transform(
            _symbols.begin(), 
            _symbols.end(),
            result.begin(),
            [](const constSym8& x) { return x.rows_iter(); }
        );
        return ChainedIter(std::move(result));
    }

    void append(const constSym8& symbol) {
        _symbols.push_back(symbol);
        setCols();
    }

    void clear() {
      _symbols.clear();
      setCols();
    }
};

#define DEFINE_SYM(NAME, ...) \
constexpr char raw##NAME[] = {__VA_ARGS__}; \
constexpr constSym8 NAME(raw##NAME);

DEFINE_SYM(SPACE,
  0, 0, 0, 0, 0, 0, 0, 0,
); 

DEFINE_SYM(ZERO,
  0, 0, 1, 1, 1, 1, 1, 0,
  0, 1, 0, 0, 0, 0, 0, 1,
  0, 1, 0, 0, 0, 0, 0, 1,
  0, 1, 0, 0, 0, 0, 0, 1,
  0, 0, 1, 1, 1, 1, 1, 0,
);

DEFINE_SYM(ONE,
  0, 0, 0, 1, 0, 0, 0, 1,
  0, 0, 1, 0, 0, 0, 0, 1,
  0, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 1,
);

DEFINE_SYM(TWO,
  0, 0, 1, 1, 0, 0, 0, 1,
  0, 1, 0, 0, 0, 0, 1, 1, 
  0, 1, 0, 0, 0, 1, 0, 1,
  0, 1, 0, 0, 1 ,0, 0, 1,
  0, 1, 1, 1, 0, 0, 0, 1,
);

DEFINE_SYM(THREE,
  0, 0, 1, 0, 0, 0, 1, 0,
  0, 1, 0, 0, 0, 0, 0, 1,
  0, 1, 0, 0, 1, 0, 0, 1,
  0, 1, 0, 1, 0, 1, 0, 1,
  0, 0, 1, 0, 0, 0, 1, 0,
);

DEFINE_SYM(FOUR,
  0, 1, 1, 1, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 1, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1,
);

DEFINE_SYM(FIVE,
  0, 1, 1, 1, 0, 0, 0, 1,
  0, 1, 0, 1, 0, 0, 0, 1,
  0, 1, 0, 1, 0, 0, 0, 1,
  0, 1, 0, 0, 1, 1, 1, 0,
);

DEFINE_SYM(SIX,
  0, 0, 0, 1, 1, 1, 1, 1,
  0, 0, 1, 0, 0, 1, 0, 1,
  0, 1, 0, 0, 1, 0, 0, 1,
  0, 0, 0, 0, 1, 0, 0, 1,
  0, 0, 0, 0, 0, 1, 1, 0,
);

DEFINE_SYM(SEVEN,
  0, 1, 0, 0, 1, 0, 0, 0,
  0, 1, 0, 0, 1, 0, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 1, 0, 0, 0,
);

DEFINE_SYM(EIGHT,
  0, 0, 1, 0, 0, 0, 1, 0,
  0, 1, 0, 1, 0, 1, 0, 1,
  0, 1, 0, 0, 1, 0, 0, 1,
  0, 1, 0, 0, 1, 0, 0, 1,
  0, 1, 0, 1, 0, 1, 0, 1,
  0, 0, 1, 0, 0, 0, 1, 0,
);

DEFINE_SYM(NINE,
  0, 0, 1, 1, 0, 0, 0, 0,
  0, 1, 0, 0, 1, 0, 0, 0,
  0, 1, 0, 0, 1, 0, 0, 1,
  0, 1, 0, 1, 0, 0, 1, 0,
  0, 1, 1, 1, 1, 1, 0, 0,
);

DEFINE_SYM(ONE_TRAIN, 
  0, 0, 1, 1, 1, 1, 0, 0,
  0, 1, 1, 1, 1, 1, 1, 0, 
  1, 1, 1, 0, 1, 1, 1, 1, 
  1, 1, 0, 1, 1, 1, 1, 1, 
  1, 0, 0, 0, 0, 0, 0, 1,
  1, 1, 1, 1, 1, 1, 1, 1,
  0, 1, 1, 1, 1, 1, 1, 0,
  0, 0, 1, 1, 1, 1, 0, 0,
); 

DEFINE_SYM(ARROW_UP,
  0, 0, 1, 0, 0, 0, 0, 0,
  0, 1, 1, 0, 0, 0, 0, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  0, 1, 1, 0, 0, 0, 0, 0,
  0, 0, 1, 0, 0, 0, 0, 0,
);

DEFINE_SYM(ARROW_DOWN,
  0, 0, 0, 0, 0, 1, 0, 0,
  0, 0, 0, 0, 0, 1, 1, 0,
  1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 1, 1, 0,
  0, 0, 0, 0, 0, 1, 0, 0,
);

const constSym8* const SYM_NUMERIC[] = {
  &ZERO, &ONE, &TWO, &THREE, &FOUR, &FIVE, &SIX, &SEVEN, &EIGHT, &NINE
};

void addNumberToArray(SymbolArray& symb, int n) { 
  if (n == 0) return;
  addNumberToArray(symb, n / 10);

  auto x = n % 10;
  symb.append(*SYM_NUMERIC[x]);
  symb.append(SPACE);
}

}
