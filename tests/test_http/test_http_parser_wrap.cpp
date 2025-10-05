#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "http_parser_wrap.h"

TEST(HttpParserWrapTest, RingBufferRequest01) {
  HttpParserWrap<RingBuffer> h_parser;
  RingBuffer buf;
  int res = h_parser.SetData(&buf, http_parser_type::HTTP_REQUEST);
  EXPECT_EQ(res, 0);
  const char* http_request =
      "GET /hello/world?name=test HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Accept: */*\r\n"
      "\r\n";
  buf.Write(http_request, strlen(http_request));
  EXPECT_EQ(buf.Readable(), strlen(http_request));
  res = h_parser.Parser();
  EXPECT_EQ(res, buf.Readable());
  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(h_parser.Complete());
  EXPECT_EQ(h_parser.Method(), http_method::HTTP_GET);

  size_t url_len = 0;
  const char* url_ptr = h_parser.Url(url_len);
  EXPECT_TRUE(url_ptr != nullptr);
  EXPECT_STREQ(std::string(url_ptr, url_len).c_str(), "/hello/world?name=test");

  std::string url_str = h_parser.Url();
  EXPECT_STREQ(url_str.c_str(), "/hello/world?name=test");

  size_t val_size = 0;
  const char* val_ptr_01 = h_parser.Headers("Host", val_size);
  ASSERT_TRUE(val_ptr_01 != nullptr);
  EXPECT_STREQ(std::string(val_ptr_01, val_size).c_str(), "example.com");

  val_size = 0;
  const char* val_ptr_02 = h_parser.Headers("User-Agent", val_size);
  ASSERT_TRUE(val_ptr_02 != nullptr);
  EXPECT_STREQ(std::string(val_ptr_02, val_size).c_str(), "curl/7.68.0");

  val_size = 0;
  const char* val_ptr_03 = h_parser.Headers("Accept", val_size);
  ASSERT_TRUE(val_ptr_03 != nullptr);
  EXPECT_STREQ(std::string(val_ptr_03, val_size).c_str(), "*/*");

  val_size = 1;
  const char* val_ptr_04 = h_parser.Headers("Acceptt", val_size);
  EXPECT_EQ(val_size, 0);
  EXPECT_TRUE(val_ptr_04 == nullptr);

  val_size = 1;
  const char* val_ptr_05 = h_parser.Headers("Unknow", val_size);
  EXPECT_EQ(val_size, 0);
  EXPECT_TRUE(val_ptr_05 == nullptr);

  bool is_exist = false;
  std::string val_str_01 = h_parser.Headers("Host", &is_exist);
  EXPECT_TRUE(is_exist);
  EXPECT_STREQ(val_str_01.c_str(), "example.com");

  is_exist = false;
  std::string val_str_02 = h_parser.Headers("User-Agent", &is_exist);
  EXPECT_TRUE(is_exist);
  EXPECT_STREQ(val_str_02.c_str(), "curl/7.68.0");

  is_exist = false;
  std::string val_str_03 = h_parser.Headers("Accept", &is_exist);
  EXPECT_TRUE(is_exist);
  EXPECT_STREQ(val_str_03.c_str(), "*/*");

  is_exist = true;
  std::string val_str_04 = h_parser.Headers("UserAgent", &is_exist);
  EXPECT_TRUE(!is_exist);
  EXPECT_STREQ(val_str_04.c_str(), "");

  is_exist = true;
  std::string val_str_05 = h_parser.Headers("Unknow", &is_exist);
  EXPECT_TRUE(!is_exist);
  EXPECT_STREQ(val_str_05.c_str(), "");
}

TEST(HttpParserWrapTest, RingBufferRequest02) {
  HttpParserWrap<RingBuffer> h_parser;
  RingBuffer buf;
  int res = h_parser.SetData(&buf, http_parser_type::HTTP_REQUEST);
  EXPECT_EQ(res, 0);
  const char* http_request =
      "POST /hello/world?name=test HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Accept: */*\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 24\r\n"
      "\r\n"
      "{\"id\":123,\"msg\":\"hello\"}";
  buf.Write(http_request, strlen(http_request));
  EXPECT_EQ(buf.Readable(), strlen(http_request));
  res = h_parser.Parser();
  EXPECT_EQ(res, buf.Readable());
  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(h_parser.Complete());
  EXPECT_EQ(h_parser.Method(), http_method::HTTP_POST);

  std::string body_str = h_parser.Body();
  EXPECT_STREQ(body_str.c_str(), "{\"id\":123,\"msg\":\"hello\"}");

  size_t body_len = 0;
  const char* body_ptr = h_parser.Body(body_len);
  ASSERT_TRUE(body_ptr != nullptr);
  EXPECT_STREQ(body_str.c_str(), "{\"id\":123,\"msg\":\"hello\"}");
}

TEST(HttpParserWrapTest, RingBufferRequest03) {
  HttpParserWrap<RingBuffer> h_parser;
  RingBuffer buf;
  int res = h_parser.SetData(&buf, http_parser_type::HTTP_REQUEST);
  EXPECT_EQ(res, 0);
  const char* http_request =
      "POST /hello/world?name=test HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Accept: */*\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 25\r\n"  // diff
      "\r\n"
      "{\"id\":123,\"msg\":\"hello\"}";
  buf.Write(http_request, strlen(http_request));
  EXPECT_EQ(buf.Readable(), strlen(http_request));
  res = h_parser.Parser();
  EXPECT_EQ(res, buf.Readable());
  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(!h_parser.Complete());  // diff
  EXPECT_EQ(h_parser.Method(), http_method::HTTP_POST);

  bool is_exist = false;
  std::string body_str = h_parser.Body(&is_exist);
  EXPECT_TRUE(is_exist);
  EXPECT_STREQ(body_str.c_str(), "{\"id\":123,\"msg\":\"hello\"}");

  size_t body_len = 0;
  const char* body_ptr = h_parser.Body(body_len);
  ASSERT_TRUE(body_ptr != nullptr);
  EXPECT_STREQ(body_str.c_str(), "{\"id\":123,\"msg\":\"hello\"}");
}

TEST(HttpParserWrapTest, RingBufferRequest04) {
  HttpParserWrap<RingBuffer> h_parser;
  RingBuffer buf;
  int res = h_parser.SetData(&buf, http_parser_type::HTTP_REQUEST);
  EXPECT_EQ(res, 0);
  const char* http_request_part01 =
      "POST /hello/world?name=test HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Acc";
  const char* http_request_part02 =
      "ept: */*\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 24\r\n"  // diff
      "\r\n"
      "{\"id\":123,\"msg\":\"hello\"}";
  buf.Write(http_request_part01, strlen(http_request_part01));
  EXPECT_EQ(buf.Readable(), strlen(http_request_part01));
  res = h_parser.Parser();
  EXPECT_EQ(res, buf.Readable());
  EXPECT_TRUE(!h_parser.HeaderComplete());
  EXPECT_TRUE(!h_parser.Complete());
  EXPECT_EQ(h_parser.Method(), http_method::HTTP_POST);

  size_t val_size = 0;
  const char* val_ptr = h_parser.Headers("Host", val_size);
  ASSERT_TRUE(val_ptr != nullptr);
  EXPECT_STREQ(std::string(val_ptr, val_size).c_str(), "example.com");

  bool is_exist = false;
  std::string val_str_01 = h_parser.Headers("User-Agent", &is_exist);
  EXPECT_TRUE(is_exist);
  EXPECT_STREQ(val_str_01.c_str(), "curl/7.68.0");

  is_exist = false;
  std::string val_str_02 = h_parser.Headers("Accept", &is_exist);
  EXPECT_TRUE(!is_exist);

  buf.Write(http_request_part02, strlen(http_request_part02));
  EXPECT_EQ(buf.Readable(),
            strlen(http_request_part01) + strlen(http_request_part02));
  res = h_parser.Parser();
  EXPECT_EQ(res, strlen(http_request_part02));
  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(h_parser.Complete());

  is_exist = false;
  val_str_02 = h_parser.Headers("Accept", &is_exist);
  EXPECT_TRUE(is_exist);
  EXPECT_STREQ(val_str_02.c_str(), "*/*");
}

TEST(HttpParserWrapTest, RingBufferRequest05) {
  HttpParserWrap<RingBuffer> h_parser;
  RingBuffer buf;
  int res = h_parser.SetData(&buf, http_parser_type::HTTP_REQUEST);
  EXPECT_EQ(res, 0);
  std::vector<std::string> str_vec(15);
  str_vec[0] = "PO";
  str_vec[1] = "ST /hel";
  str_vec[2] = "lo/world?name";
  str_vec[3] =
      "=test HTTP/1.1\r\n"
      "Host: exa";
  str_vec[4] =
      "mple.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Acc";
  str_vec[5] = "ept: */*\r";
  str_vec[6] = "\n";
  str_vec[7] = "Content-Type: application/json";
  str_vec[8] = "\r";
  str_vec[9] =
      "\n"
      "Content-Length: 2";
  str_vec[10] = "4\r\n";
  str_vec[11] = "\r\n";
  str_vec[12] = "{\"id\":";
  str_vec[13] = "123,\"msg";
  str_vec[14] = "\":\"hello\"}";

  for (const std::string part : str_vec) {
    buf.Write(part.c_str(), part.size());
    int part_len = h_parser.Parser();
    ASSERT_TRUE(part_len > 0);
    EXPECT_EQ(part_len, part.size());
  }

  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(h_parser.Complete());
  EXPECT_EQ(h_parser.Method(), http_method::HTTP_POST);
  EXPECT_STREQ(h_parser.Url().c_str(), "/hello/world?name=test");
  EXPECT_STREQ(h_parser.Headers("Host").c_str(), "example.com");
  EXPECT_STREQ(h_parser.Headers("User-Agent").c_str(), "curl/7.68.0");
  EXPECT_STREQ(h_parser.Headers("Accept").c_str(), "*/*");
  EXPECT_STREQ(h_parser.Headers("Content-Type").c_str(), "application/json");
  EXPECT_STREQ(h_parser.Headers("Content-Length").c_str(), "24");
  EXPECT_STREQ(h_parser.Body().c_str(), "{\"id\":123,\"msg\":\"hello\"}");
}

TEST(HttpParserWrapTest, RingBufferRequest06) {
  HttpParserWrap<RingBuffer> h_parser;
  RingBuffer buf;
  int res = h_parser.SetData(&buf, http_parser_type::HTTP_REQUEST);
  EXPECT_EQ(res, 0);
  const char* http_request01 =
      "GET /hello/world?name=test HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Accept: */*\r\n"
      "\r\n";
  buf.Write(http_request01, strlen(http_request01));
  EXPECT_EQ(buf.Readable(), strlen(http_request01));
  res = h_parser.Parser();
  EXPECT_EQ(res, buf.Readable());
  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(h_parser.Complete());
  EXPECT_EQ(h_parser.Method(), http_method::HTTP_GET);

  size_t url_len = 0;
  const char* url_ptr = h_parser.Url(url_len);
  EXPECT_TRUE(url_ptr != nullptr);
  EXPECT_STREQ(std::string(url_ptr, url_len).c_str(), "/hello/world?name=test");

  buf.Clear();
  res = h_parser.SetData(&buf, http_parser_type::HTTP_REQUEST);
  EXPECT_EQ(res, 0);
  const char* http_request02 =
      "POST /hello/world?name=test HTTP/1.1\r\n"
      "Host: example.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Accept: */*\r\n"
      "Content-Type: application/json\r\n"
      "Content-Length: 24\r\n"
      "\r\n"
      "{\"id\":123,\"msg\":\"hello\"}";
  buf.Write(http_request02, strlen(http_request02));
  EXPECT_EQ(buf.Readable(), strlen(http_request02));
  res = h_parser.Parser();
  EXPECT_EQ(res, buf.Readable());
  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(h_parser.Complete());
  EXPECT_EQ(h_parser.Method(), http_method::HTTP_POST);

  std::string body_str = h_parser.Body();
  EXPECT_STREQ(body_str.c_str(), "{\"id\":123,\"msg\":\"hello\"}");
}

TEST(HttpParserWrapTest, StdStringRequest) {
  HttpParserWrap<std::string> h_parser;
  std::string buf;
  int res = h_parser.SetData(&buf, http_parser_type::HTTP_REQUEST);
  EXPECT_EQ(res, 0);
  std::vector<std::string> str_vec(15);
  str_vec[0] = "PO";
  str_vec[1] = "ST /hel";
  str_vec[2] = "lo/world?name";
  str_vec[3] =
      "=test HTTP/1.1\r\n"
      "Host: exa";
  str_vec[4] =
      "mple.com\r\n"
      "User-Agent: curl/7.68.0\r\n"
      "Acc";
  str_vec[5] = "ept: */*\r";
  str_vec[6] = "\n";
  str_vec[7] = "Content-Type: application/json";
  str_vec[8] = "\r";
  str_vec[9] =
      "\n"
      "Content-Length: 2";
  str_vec[10] = "4\r\n";
  str_vec[11] = "\r\n";
  str_vec[12] = "{\"id\":";
  str_vec[13] = "123,\"msg";
  str_vec[14] = "\":\"hello\"}";

  for (const std::string part : str_vec) {
    buf.append(part);
    int part_len = h_parser.Parser();
    ASSERT_TRUE(part_len > 0);
    EXPECT_EQ(part_len, part.size());
  }

  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(h_parser.Complete());
  EXPECT_EQ(h_parser.Method(), http_method::HTTP_POST);
  EXPECT_STREQ(h_parser.Url().c_str(), "/hello/world?name=test");
  EXPECT_STREQ(h_parser.Headers("Host").c_str(), "example.com");
  EXPECT_STREQ(h_parser.Headers("User-Agent").c_str(), "curl/7.68.0");
  EXPECT_STREQ(h_parser.Headers("Accept").c_str(), "*/*");
  EXPECT_STREQ(h_parser.Headers("Content-Type").c_str(), "application/json");
  EXPECT_STREQ(h_parser.Headers("Content-Length").c_str(), "24");
  EXPECT_STREQ(h_parser.Body().c_str(), "{\"id\":123,\"msg\":\"hello\"}");
}

TEST(HttpParserWrapTest, RingBufferResponse) {
  HttpParserWrap<RingBuffer> h_parser;
  RingBuffer buf;
  int res = h_parser.SetData(&buf, http_parser_type::HTTP_RESPONSE);
  EXPECT_EQ(res, 0);
  std::vector<std::string> str_vec(22);
  str_vec[0] = "HT";
  str_vec[1] = "TP";
  str_vec[2] = "/1.1 2";
  str_vec[3] = "00 OK\r";
  str_vec[4] = "\n";
  str_vec[5] = "Date";
  str_vec[6] = ":";
  str_vec[7] = " ";
  str_vec[8] = "Sun, 05 Oct 2";
  str_vec[9] = "025";
  str_vec[10] = " 12:34:56 GMT\r\n";
  str_vec[11] = "Server: MyTestServer/1.0\r\n";
  str_vec[12] = "Content-";
  str_vec[13] = "Type: application";
  str_vec[14] = "/";
  str_vec[15] = "json\r\n";
  str_vec[16] = "Content-Length: 38\r\n";
  str_vec[17] = "Connection: cl";
  str_vec[18] = "ose\r\n";
  str_vec[19] = "\r\n";
  str_vec[20] = "{\"status\":\"ok\",\"me";
  str_vec[21] = "ssage\":\"hello back\"}";

  for (const std::string part : str_vec) {
    buf.Write(part.c_str(), part.size());
    int part_len = h_parser.Parser();
    ASSERT_TRUE(part_len > 0);
    EXPECT_EQ(part_len, part.size());
  }

  const char* http_response =
    "HTTP/1.1 200 OK\r\n"
    "Date: Sun, 05 Oct 2025 12:34:56 GMT\r\n"
    "Server: MyTestServer/1.0\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 38\r\n"
    "Connection: close\r\n"
    "\r\n"
    "{\"status\":\"ok\",\"message\":\"hello back\"}";

  EXPECT_TRUE(h_parser.HeaderComplete());
  EXPECT_TRUE(h_parser.Complete());
  EXPECT_EQ(h_parser.StatusCode(), 200);
  EXPECT_STREQ(h_parser.Status().c_str(), "OK");
  EXPECT_STREQ(h_parser.Headers("Date").c_str(), "Sun, 05 Oct 2025 12:34:56 GMT");
  EXPECT_STREQ(h_parser.Headers("Server").c_str(), "MyTestServer/1.0");
  EXPECT_STREQ(h_parser.Headers("Content-Type").c_str(), "application/json");
  EXPECT_STREQ(h_parser.Headers("Content-Length").c_str(), "38");
  EXPECT_STREQ(h_parser.Headers("Connection").c_str(), "close");
  EXPECT_STREQ(h_parser.Body().c_str(), "{\"status\":\"ok\",\"message\":\"hello back\"}");
}
