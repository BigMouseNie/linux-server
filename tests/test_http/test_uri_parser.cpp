#include <gtest/gtest.h>

#include "uri_parser.h"

// scheme:[//authority]path[?query][#fragment]

TEST(UriParserTest, UriParser01) {
  UriParser u_parser(
      "https://example.com/shop;category=books;lang=en/"
      "item;id=123;discount=true");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "https");
  EXPECT_STREQ(u_parser.Host().c_str(), "example.com");
  EXPECT_STREQ(u_parser.Path().c_str(), "shop/item");
  EXPECT_STREQ(u_parser.PathWithParams().c_str(),
               "shop;category=books;lang=en/item;id=123;discount=true");
  const auto& ppkv = u_parser.GetPathParamsKV();
  ASSERT_EQ(ppkv.size(), 2);
  EXPECT_STREQ(ppkv[0].first.c_str(), "shop");
  const auto& kv01 = ppkv[0].second;
  ASSERT_TRUE(kv01.find("category") != kv01.end());
  ASSERT_TRUE(kv01.find("lang") != kv01.end());
  EXPECT_STREQ(kv01.at("category").c_str(), "books");
  EXPECT_STREQ(kv01.at("lang").c_str(), "en");

  EXPECT_STREQ(ppkv[1].first.c_str(), "item");
  const auto& kv02 = ppkv[1].second;
  ASSERT_TRUE(kv02.find("id") != kv02.end());
  ASSERT_TRUE(kv02.find("discount") != kv02.end());
  EXPECT_STREQ(kv02.at("id").c_str(), "123");
  EXPECT_STREQ(kv02.at("discount").c_str(), "true");
}

TEST(UriParserTest, UriParser02) {
  UriParser u_parser("http://example.com/page#section2");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "http");
  EXPECT_STREQ(u_parser.Host().c_str(), "example.com");
  EXPECT_STREQ(u_parser.Path().c_str(), "page");
  EXPECT_STREQ(u_parser.PathWithParams().c_str(), "page");
  const auto& ppkv = u_parser.GetPathParamsKV();
  ASSERT_EQ(ppkv.size(), 1);
  EXPECT_STREQ(ppkv[0].first.c_str(), "page");
  EXPECT_STREQ(u_parser.Fragment().c_str(), "section2");
}

TEST(UriParserTest, UriParser03) {
  UriParser u_parser("https://user:pass@secure.example.com/login");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "https");
  EXPECT_STREQ(u_parser.UserInfo().c_str(), "user:pass");
  EXPECT_STREQ(u_parser.Host().c_str(), "secure.example.com");
  EXPECT_STREQ(u_parser.Path().c_str(), "login");
}

TEST(UriParserTest, UriParser04) {
  UriParser u_parser("http://example.com:8080/api/v1");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "http");
  EXPECT_STREQ(u_parser.Host().c_str(), "example.com");
  EXPECT_STREQ(u_parser.Port().c_str(), "8080");
}

TEST(UriParserTest, UriParser05) {
  UriParser u_parser("https://example.com/item;id=123");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Path().c_str(), "item");
  EXPECT_STREQ(u_parser.PathWithParams().c_str(), "item;id=123");
  const auto& ppkv = u_parser.GetPathParamsKV();
  ASSERT_EQ(ppkv.size(), 1);
  EXPECT_STREQ(ppkv[0].first.c_str(), "item");
  const auto& kv01 = ppkv[0].second;
  ASSERT_TRUE(kv01.find("id") != kv01.end());
  EXPECT_STREQ(kv01.at("id").c_str(), "123");
}

TEST(UriParserTest, UriParser06) {
  UriParser u_parser("file:/usr/local/bin");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "file");
  EXPECT_STREQ(u_parser.Path().c_str(), "usr/local/bin");
}

TEST(UriParserTest, UriParser07) {
  UriParser u_parser("https://example.com/foo//bar");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "https");
  EXPECT_STREQ(u_parser.Path().c_str(), "foo//bar");
  const auto& ppkv = u_parser.GetPathParamsKV();
  ASSERT_EQ(ppkv.size(), 3);
  EXPECT_STREQ(ppkv[1].first.c_str(), "");
}

TEST(UriParserTest, UriParser08) {
  UriParser u_parser("https://example.com/path/;");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "https");
  EXPECT_STREQ(u_parser.Path().c_str(), "path/");
  const auto& ppkv = u_parser.GetPathParamsKV();
  ASSERT_EQ(ppkv.size(), 2);
  EXPECT_STREQ(ppkv[1].first.c_str(), "");
}

TEST(UriParserTest, UriParser09) {
  UriParser u_parser("/local/path/to/file");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "");
  EXPECT_STREQ(u_parser.Path().c_str(), "local/path/to/file");
  const auto& ppkv = u_parser.GetPathParamsKV();
  ASSERT_EQ(ppkv.size(), 4);
}

TEST(UriParserTest, UriParser10) {
  UriParser u_parser("data:text/plain;base64,SGVsbG8sIFdvcmxkIQ==");
  ASSERT_TRUE(u_parser.IsValid());
  EXPECT_STREQ(u_parser.Scheme().c_str(), "data");
  EXPECT_STREQ(u_parser.Path().c_str(), "text/plain");
  const auto& pp = u_parser.GetPathParams();
  ASSERT_EQ(pp.size(), 2);
  const auto& paeam01 = pp[1].second;
  EXPECT_STREQ(pp[1].second.c_str(), "base64,SGVsbG8sIFdvcmxkIQ==");
}
