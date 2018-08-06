/*******************************************************************************
 * This file is part of the "Enduro2D"
 * For conditions of distribution and use, see copyright notice in LICENSE.md
 * Copyright (C) 2018 Matvey Cherevko
 ******************************************************************************/

#include "_utils.hpp"
using namespace e2d;

TEST_CASE("strings") {
    {
        REQUIRE(make_utf8("hello") == "hello");
        REQUIRE(make_utf8(L"hello") == "hello");
        REQUIRE(make_utf8(u"hello") == "hello");
        REQUIRE(make_utf8(U"hello") == "hello");

        REQUIRE(make_wide("hello") == L"hello");
        REQUIRE(make_wide(L"hello") == L"hello");
        REQUIRE(make_wide(u"hello") == L"hello");
        REQUIRE(make_wide(U"hello") == L"hello");

        REQUIRE(make_utf16("hello") == u"hello");
        REQUIRE(make_utf16(L"hello") == u"hello");
        REQUIRE(make_utf16(u"hello") == u"hello");
        REQUIRE(make_utf16(U"hello") == u"hello");

        REQUIRE(make_utf32("hello") == U"hello");
        REQUIRE(make_utf32(L"hello") == U"hello");
        REQUIRE(make_utf32(u"hello") == U"hello");
        REQUIRE(make_utf32(U"hello") == U"hello");
    }
    {
        const char* null_utf8 = nullptr;
        const wchar_t* null_wide = nullptr;
        const char16_t* null_utf16 = nullptr;
        const char32_t* null_utf32 = nullptr;

        REQUIRE(make_utf8(str_view(null_utf8, 0)) == make_utf8(u""));
        REQUIRE(make_utf8(wstr_view(null_wide, 0)) == make_utf8(L""));
        REQUIRE(make_utf8(str16_view(null_utf16, 0)) == make_utf8(u""));
        REQUIRE(make_utf8(str32_view(null_utf32, 0)) == make_utf8(U""));

        REQUIRE(make_wide(str_view(null_utf8, 0)) == make_wide(u""));
        REQUIRE(make_wide(wstr_view(null_wide, 0)) == make_wide(L""));
        REQUIRE(make_wide(str16_view(null_utf16, 0)) == make_wide(u""));
        REQUIRE(make_wide(str32_view(null_utf32, 0)) == make_wide(U""));

        REQUIRE(make_utf16(str_view(null_utf8, 0)) == make_utf16(u""));
        REQUIRE(make_utf16(wstr_view(null_wide, 0)) == make_utf16(L""));
        REQUIRE(make_utf16(str16_view(null_utf16, 0)) == make_utf16(u""));
        REQUIRE(make_utf16(str32_view(null_utf32, 0)) == make_utf16(U""));

        REQUIRE(make_utf32(str_view(null_utf8, 0)) == make_utf32(u""));
        REQUIRE(make_utf32(wstr_view(null_wide, 0)) == make_utf32(L""));
        REQUIRE(make_utf32(str16_view(null_utf16, 0)) == make_utf32(u""));
        REQUIRE(make_utf32(str32_view(null_utf32, 0)) == make_utf32(U""));
    }
    {
        using strings::wildcard_match;

        char invalid_utf[] = "\xe6\x97\xa5\xd1\x88\xfa";
        REQUIRE_THROWS_AS(wildcard_match(invalid_utf, "???"), std::exception);

        const auto mark_string = [](const char* str) -> str_view {
            static char string_buf[1024] = {0};
            std::memset(string_buf, '*', sizeof(string_buf));
            const std::size_t str_len = std::strlen(str);
            E2D_ASSERT(str_len < sizeof(string_buf));
            std::memcpy(string_buf, str, str_len);
            return str_view(string_buf, str_len);
        };

        const auto mark_pattern = [](const char* str) -> str_view {
            static char pattern_buf[1024] = {0};
            std::memset(pattern_buf, '*', sizeof(pattern_buf));
            const std::size_t str_len = std::strlen(str);
            E2D_ASSERT(str_len < sizeof(pattern_buf));
            std::memcpy(pattern_buf, str, str_len);
            return str_view(pattern_buf, str_len);
        };

        // 你好!
        REQUIRE(wildcard_match(u8"\u4F60\u597D!", u8"\u4F60\u597D!") == true);
        REQUIRE(wildcard_match(u8"\u4F60\u597D!", u8"?\u597D!") == true);
        REQUIRE(wildcard_match(u8"\u4F60\u597D!", u8"\u4F60?!") == true);

        REQUIRE(wildcard_match(
            // 你好你好你好你好世界世界世界世界世界世界世界世界彡ಠ
            mark_string("\u4F60\u597D\u4F60\u597D\u4F60\u597D\u4F60\u597D\u4E16\u754C\u4E16\u754C\u4E16\u754C\u4E16\u754C\u4E16\u754C\u4E16\u754C\u4E16\u754C\u4E16\u754C\u5F61\u0CA0"),
            // 你好你好你好你好*世界世界彡*ಠ
            mark_pattern("\u4F60\u597D\u4F60\u597D\u4F60\u597D\u4F60\u597D*\u4E16\u754C\u4E16\u754C\u5F61*\u0CA0")) == true);

        REQUIRE(wildcard_match("", "") == true);
        REQUIRE(wildcard_match("a", "") == false);
        REQUIRE(wildcard_match("", "*") == true);
        REQUIRE(wildcard_match("", "?") == false);

        const char* null_utf8 = nullptr;
        str_view null_view = {null_utf8, 0};
        REQUIRE(wildcard_match(null_view, null_view) == true);
        REQUIRE(wildcard_match("a", null_view) == false);
        REQUIRE(wildcard_match(null_view, "*") == true);
        REQUIRE(wildcard_match(null_view, "?") == false);

        // tests source:
        // http://developforperformance.com/MatchingWildcards_AnImprovedAlgorithmForBigData.html

        REQUIRE(wildcard_match(mark_string("abc"), mark_pattern("ab*d")) == false);

        REQUIRE(wildcard_match(mark_string("abcccd"), mark_pattern("*ccd")) == true);
        REQUIRE(wildcard_match(mark_string("mississipissippi"), mark_pattern("*issip*ss*")) == true);
        REQUIRE(wildcard_match(mark_string("xxxx*zzzzzzzzy*f"), mark_pattern("xxxx*zzy*fffff")) == false);
        REQUIRE(wildcard_match(mark_string("xxxx*zzzzzzzzy*f"), mark_pattern("xxx*zzy*f")) == true);
        REQUIRE(wildcard_match(mark_string("xxxxzzzzzzzzyf"), mark_pattern("xxxx*zzy*fffff")) == false);
        REQUIRE(wildcard_match(mark_string("xxxxzzzzzzzzyf"), mark_pattern("xxxx*zzy*f")) == true);
        REQUIRE(wildcard_match(mark_string("xyxyxyzyxyz"), mark_pattern("xy*z*xyz")) == true);
        REQUIRE(wildcard_match(mark_string("mississippi"), mark_pattern("*sip*")) == true);
        REQUIRE(wildcard_match(mark_string("xyxyxyxyz"), mark_pattern("xy*xyz")) == true);
        REQUIRE(wildcard_match(mark_string("mississippi"), mark_pattern("mi*sip*")) == true);
        REQUIRE(wildcard_match(mark_string("ababac"), mark_pattern("*abac*")) == true);
        REQUIRE(wildcard_match(mark_string("ababac"), mark_pattern("*abac*")) == true);
        REQUIRE(wildcard_match(mark_string("aaazz"), mark_pattern("a*zz*")) == true);
        REQUIRE(wildcard_match(mark_string("a12b12"), mark_pattern("*12*23")) == false);
        REQUIRE(wildcard_match(mark_string("a12b12"), mark_pattern("a12b")) == false);
        REQUIRE(wildcard_match(mark_string("a12b12"), mark_pattern("*12*12*")) == true);

        REQUIRE(wildcard_match(mark_string("caaab"), mark_pattern("*a?b")) == true);

        REQUIRE(wildcard_match(mark_string("*"), mark_pattern("*")) == true);
        REQUIRE(wildcard_match(mark_string("a*abab"), mark_pattern("a*b")) == true);
        REQUIRE(wildcard_match(mark_string("a*r"), mark_pattern("a*")) == true);
        REQUIRE(wildcard_match(mark_string("a*ar"), mark_pattern("a*aar")) == false);

        REQUIRE(wildcard_match(mark_string("XYXYXYZYXYz"), mark_pattern("XY*Z*XYz")) == true);
        REQUIRE(wildcard_match(mark_string("missisSIPpi"), mark_pattern("*SIP*")) == true);
        REQUIRE(wildcard_match(mark_string("mississipPI"), mark_pattern("*issip*PI")) == true);
        REQUIRE(wildcard_match(mark_string("xyxyxyxyz"), mark_pattern("xy*xyz")) == true);
        REQUIRE(wildcard_match(mark_string("miSsissippi"), mark_pattern("mi*sip*")) == true);
        REQUIRE(wildcard_match(mark_string("miSsissippi"), mark_pattern("mi*Sip*")) == false);
        REQUIRE(wildcard_match(mark_string("abAbac"), mark_pattern("*Abac*")) == true);
        REQUIRE(wildcard_match(mark_string("abAbac"), mark_pattern("*Abac*")) == true);
        REQUIRE(wildcard_match(mark_string("aAazz"), mark_pattern("a*zz*")) == true);
        REQUIRE(wildcard_match(mark_string("A12b12"), mark_pattern("*12*23")) == false);
        REQUIRE(wildcard_match(mark_string("a12B12"), mark_pattern("*12*12*")) == true);
        REQUIRE(wildcard_match(mark_string("oWn"), mark_pattern("*oWn*")) == true);

        REQUIRE(wildcard_match(mark_string("bLah"), mark_pattern("bLah")) == true);
        REQUIRE(wildcard_match(mark_string("bLah"), mark_pattern("bLaH")) == false);

        REQUIRE(wildcard_match(mark_string("a"), mark_pattern("*?")) == true);
        REQUIRE(wildcard_match(mark_string("ab"), mark_pattern("*?")) == true);
        REQUIRE(wildcard_match(mark_string("abc"), mark_pattern("*?")) == true);

        REQUIRE(wildcard_match(mark_string("a"), mark_pattern("??")) == false);
        REQUIRE(wildcard_match(mark_string("ab"), mark_pattern("?*?")) == true);
        REQUIRE(wildcard_match(mark_string("ab"), mark_pattern("*?*?*")) == true);
        REQUIRE(wildcard_match(mark_string("abc"), mark_pattern("?**?*?")) == true);
        REQUIRE(wildcard_match(mark_string("abc"), mark_pattern("?**?*&?")) == false);
        REQUIRE(wildcard_match(mark_string("abcd"), mark_pattern("?b*??")) == true);
        REQUIRE(wildcard_match(mark_string("abcd"), mark_pattern("?a*??")) == false);
        REQUIRE(wildcard_match(mark_string("abcd"), mark_pattern("?**?c?")) == true);
        REQUIRE(wildcard_match(mark_string("abcd"), mark_pattern("?**?d?")) == false);
        REQUIRE(wildcard_match(mark_string("abcde"), mark_pattern("?*b*?*d*?")) == true);

        REQUIRE(wildcard_match(mark_string("bLah"), mark_pattern("bL?h")) == true);
        REQUIRE(wildcard_match(mark_string("bLaaa"), mark_pattern("bLa?")) == false);
        REQUIRE(wildcard_match(mark_string("bLah"), mark_pattern("bLa?")) == true);
        REQUIRE(wildcard_match(mark_string("bLaH"), mark_pattern("?Lah")) == false);
        REQUIRE(wildcard_match(mark_string("bLaH"), mark_pattern("?LaH")) == true);

        REQUIRE(wildcard_match(
            mark_string("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaab"),
            mark_pattern("a*a*a*a*a*a*aa*aaa*a*a*b")) == true);
        REQUIRE(wildcard_match(
            mark_string("abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab"),
            mark_pattern("*a*b*ba*ca*a*aa*aaa*fa*ga*b*")) == true);
        REQUIRE(wildcard_match(
            mark_string("abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab"),
            mark_pattern("*a*b*ba*ca*a*x*aaa*fa*ga*b*")) == false);
        REQUIRE(wildcard_match(
            mark_string("abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab"),
            mark_pattern("*a*b*ba*ca*aaaa*fa*ga*gggg*b*")) == false);
        REQUIRE(wildcard_match(
            mark_string("abababababababababababababababababababaacacacacacacacadaeafagahaiajakalaaaaaaaaaaaaaaaaaffafagaagggagaaaaaaaab"),
            mark_pattern("*a*b*ba*ca*aaaa*fa*ga*ggg*b*")) == true);
        REQUIRE(wildcard_match(
            mark_string("aaabbaabbaab"),
            mark_pattern("*aabbaa*a*")) == true);
        REQUIRE(wildcard_match(
            mark_string("a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*"),
            mark_pattern("a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*")) == true);
        REQUIRE(wildcard_match(
            mark_string("aaaaaaaaaaaaaaaaa"),
            mark_pattern("*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*")) == true);
        REQUIRE(wildcard_match(
            mark_string("aaaaaaaaaaaaaaaa"),
            mark_pattern("*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*a*")) == false);
        REQUIRE(wildcard_match(
            mark_string("abc*abcd*abcde*abcdef*abcdefg*abcdefgh*abcdefghi*abcdefghij*abcdefghijk*abcdefghijkl*abcdefghijklm*abcdefghijklmn"),
            mark_pattern("abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*")) == false);
        REQUIRE(wildcard_match(
            mark_string("abc*abcd*abcde*abcdef*abcdefg*abcdefgh*abcdefghi*abcdefghij*abcdefghijk*abcdefghijkl*abcdefghijklm*abcdefghijklmn"),
            mark_pattern("abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*")) == true);
        REQUIRE(wildcard_match(
            mark_string("abc*abcd*abcd*abc*abcd"),
            mark_pattern("abc*abc*abc*abc*abc")) == false);
        REQUIRE(wildcard_match(
            mark_string("abc*abcd*abcd*abc*abcd*abcd*abc*abcd*abc*abc*abcd"),
            mark_pattern("abc*abc*abc*abc*abc*abc*abc*abc*abc*abc*abcd")) == true);
        REQUIRE(wildcard_match(mark_string("abc"), mark_pattern("********a********b********c********")) == true);
        REQUIRE(wildcard_match(mark_string("********a********b********c********"), mark_pattern("abc")) == false);
        REQUIRE(wildcard_match(mark_string("abc"), mark_pattern("********a********b********b********")) == false);
        REQUIRE(wildcard_match(mark_string("*abc*"), mark_pattern("***a*b*c***")) == true);
    }
    {
        char buf[6];
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), nullptr), strings::bad_format);
        REQUIRE_THROWS_AS(strings::format(buf, 0, "hello"), strings::bad_format_buffer);
        REQUIRE_THROWS_AS(strings::format(nullptr, sizeof(buf), "hello"), strings::bad_format_buffer);
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "helloE"), strings::bad_format_buffer);
        REQUIRE_NOTHROW(strings::format(buf, sizeof(buf), "hello"));
        REQUIRE_NOTHROW(strings::format(nullptr, 0, "hello"));

        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "%hell"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "he%ll"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "hell%"), strings::bad_format);

        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "%10%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "hell%10%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "%10%hell"), strings::bad_format);

        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "%x%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "hell%y%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "%z%hell"), strings::bad_format);
    }
    {
        REQUIRE_THROWS_AS(strings::rformat(nullptr), strings::bad_format);

        REQUIRE_THROWS_AS(strings::rformat("%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::rformat("%hell"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::rformat("he%ll"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::rformat("hell%"), strings::bad_format);

        REQUIRE_THROWS_AS(strings::rformat("%10%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::rformat("hell%10%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::rformat("%10%hell"), strings::bad_format);

        REQUIRE_THROWS_AS(strings::rformat("%x%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::rformat("hell%y%"), strings::bad_format);
        REQUIRE_THROWS_AS(strings::rformat("%z%hell"), strings::bad_format);
    }
    {
        char buf[1];

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE(strings::format(buf, sizeof(buf), "") == 0);
        REQUIRE(str(buf) == str(""));
    }
    {
        char buf[6];

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE(strings::format(buf, sizeof(buf), "hello") == 5);
        REQUIRE(strings::format(nullptr, 0, "hello") == 5);
        REQUIRE(str(buf) == str("hello"));

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE(strings::format(buf, sizeof(buf), "hell%%") == 5);
        REQUIRE(strings::format(nullptr, 0, "hell%%") == 5);
        REQUIRE(str(buf) == str("hell%"));

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE(strings::format(buf, sizeof(buf), "%%hell") == 5);
        REQUIRE(strings::format(nullptr, 0, "%%hell") == 5);
        REQUIRE(str(buf) == str("%hell"));

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE(strings::format(buf, sizeof(buf), "he%%ll") == 5);
        REQUIRE(strings::format(nullptr, 0, "he%%ll") == 5);
        REQUIRE(str(buf) == str("he%ll"));
    }
    {
        char buf[5];
        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "hello"), strings::bad_format_buffer);
        REQUIRE(str(buf) == str("hell"));

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "he%"), strings::bad_format);
        REQUIRE(str(buf) == str("he"));

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "he%99%"), strings::bad_format);
        REQUIRE(str(buf) == str("he"));

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "he%x%"), strings::bad_format);
        REQUIRE(str(buf) == str("he"));

        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "he%0%", 1234), strings::bad_format_buffer);
        REQUIRE(str(buf) == str("he12"));
    }
    {
        char buf[10];
        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "%0%", "hello world"), strings::bad_format_buffer);
        REQUIRE(str(buf) == str("hello wor"));
        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "test%0%", "hello world"), strings::bad_format_buffer);
        REQUIRE(str(buf) == str("testhello"));
        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "%0%test", "hello world"), strings::bad_format_buffer);
        REQUIRE(str(buf) == str("hello wor"));
        std::memset(buf, 0xAB, sizeof(buf));
        REQUIRE_THROWS_AS(strings::format(buf, sizeof(buf), "te%0%st", "hello world"), strings::bad_format_buffer);
        REQUIRE(str(buf) == str("tehello w"));
    }
    {
        REQUIRE(strings::rformat("%0 %1 %2", "hello", "world", 5) == str("hello world 5"));
        REQUIRE(strings::rformat("%1 %0 %2", "hello", "world", 5) == str("world hello 5"));
        REQUIRE(strings::rformat("%2 %1 %0", "hello", "world", 5) == str("5 world hello"));
        REQUIRE(strings::rformat("%0 %0 %1", "hello", "world", 5) == str("hello hello world"));
        REQUIRE(strings::rformat("%2 %1 %1", "hello", "world", 5) == str("5 world world"));

        REQUIRE(
            strings::rformat(
                "%0 %2 %1 %4 %3 %6 %7 %5 %8 %9",
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9) ==
            str("0 2 1 4 3 6 7 5 8 9"));
    }
    {
        REQUIRE(strings::rformat("%0", strings::make_format_arg(-5, 3)) == " -5");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(-5, 4)) == "  -5");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(21, 1)) == "21");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(21, 2)) == "21");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(42, 3)) == " 42");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(42u, 3)) == " 42");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(1.23f)) == "1.230000");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(1.23f,0)) == "1.230000");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(1.23f,0,2)) == "1.23");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(1.23f,5,2)) == " 1.23");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(true)) == "true");
        REQUIRE(strings::rformat("%0", strings::make_format_arg(false)) == "false");

        const char* s0 = "hello";
        char s1[64];
        std::strcpy(s1, "world");
        REQUIRE(strings::rformat("%0", s0) == "hello");
        REQUIRE(strings::rformat("%0", s1) == "world");
    }
    {
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<i8>::max()) ==
            std::to_string(std::numeric_limits<i8>::max()));
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<i8>::min()) ==
            std::to_string(std::numeric_limits<i8>::min()));
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<i64>::max()) ==
            std::to_string(std::numeric_limits<i64>::max()));
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<i64>::min()) ==
            std::to_string(std::numeric_limits<i64>::min()));
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<u64>::max()) ==
            std::to_string(std::numeric_limits<u64>::max()));
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<f32>::max()) ==
            std::to_string(std::numeric_limits<f32>::max()));
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<f32>::min()) ==
            std::to_string(std::numeric_limits<f32>::min()));
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<f64>::max()) ==
            std::to_string(std::numeric_limits<f64>::max()));
        REQUIRE(
            strings::rformat("%0", std::numeric_limits<f64>::min()) ==
            std::to_string(std::numeric_limits<f64>::min()));
    }
    SECTION("performance") {
        std::printf("-= strings::performance tests =-\n");
    #if defined(E2D_BUILD_MODE) && E2D_BUILD_MODE == E2D_BUILD_MODE_DEBUG
        const std::size_t task_n = 100'000;
    #else
        const std::size_t task_n = 1'000'000;
    #endif
        {
            std::size_t result = 0;
            e2d_untests::verbose_profiler_ms p("format(int, int)");
            for ( std::size_t i = 0; i < task_n; ++i ) {
                char buffer[128];
                result += strings::format(buffer, sizeof(buffer), "hello %0 world %1 !", 1000, 123);
            }
            p.done(result);
        }
        {
            std::ptrdiff_t result = 0;
            e2d_untests::verbose_profiler_ms p("snprintf(int, int)");
            for ( std::size_t i = 0; i < task_n; ++i ) {
                char buffer[128];
                result += std::snprintf(buffer, sizeof(buffer), "hello %i world %i !", 1000, 123);
            }
            p.done(result);
        }
        {
            std::ptrdiff_t result = 0;
            e2d_untests::verbose_profiler_ms p("format(float, float)");
            for ( std::size_t i = 0; i < task_n; ++i ) {
                char buffer[128];
                result += strings::format(buffer, sizeof(buffer), "hello %0 world %1 !", 1000.f, 123.f);
            }
            p.done(result);
        }
        {
            std::ptrdiff_t result = 0;
            e2d_untests::verbose_profiler_ms p("snprintf(float, float)");
            for ( std::size_t i = 0; i < task_n; ++i ) {
                char buffer[128];
                result += std::snprintf(buffer, sizeof(buffer), "hello %f world %f !", 1000.0, 123.0);
            }
            p.done(result);
        }
        {
            std::ptrdiff_t result = 0;
            e2d_untests::verbose_profiler_ms p("format(const char*)");
            for ( std::size_t i = 0; i < task_n; ++i ) {
                char buffer[128];
                result += strings::format(buffer, sizeof(buffer), "hello %0 world %1 !", "foo", "bar");
            }
            p.done(result);
        }
        {
            std::ptrdiff_t result = 0;
            e2d_untests::verbose_profiler_ms p("snprintf(const char*)");
            for ( std::size_t i = 0; i < task_n; ++i ) {
                char buffer[128];
                result += std::snprintf(buffer, sizeof(buffer), "hello %s world %s !", "foo", "bar");
            }
            p.done(result);
        }
    }
}