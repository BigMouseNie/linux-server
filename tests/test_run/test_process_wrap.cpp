#include <gtest/gtest.h>
#include <sys/wait.h>
#include <unistd.h>

#include "process_wrap.h"

class ProcessWrapTest : public ::testing::Test {
 protected:
  void SetUp() override {
    tmp_file_ = "/tmp/process_wrap_test_" + std::to_string(getpid()) + "_" +
                std::to_string(test_counter_++);
  }

  void TearDown() override { unlink(tmp_file_.c_str()); }

  std::string tmp_file_;

 private:
  static int test_counter_;
};

int ProcessWrapTest::test_counter_ = 0;

TEST_F(ProcessWrapTest, BasicExecution) {
  ProcessWrap proc([&tmp = tmp_file_]() {
    FILE* f = fopen(tmp.c_str(), "w");
    if (!f) return;
    fprintf(f, "child ran");
    fclose(f);
  });

  EXPECT_GT(proc.Pid(), 0);

  int status;
  waitpid(proc.Pid(), &status, 0);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(WEXITSTATUS(status), 0);

  FILE* f = fopen(tmp_file_.c_str(), "r");
  ASSERT_NE(f, nullptr);
  char buf[64] = {0};
  fgets(buf, sizeof(buf), f);
  fclose(f);
  EXPECT_STREQ(buf, "child ran");
}

TEST_F(ProcessWrapTest, WithArgs) {
  ProcessWrap proc(
      [&tmp = tmp_file_](int a, int b) {
        FILE* f = fopen(tmp.c_str(), "w");
        if (!f) return;
        fprintf(f, "%d", a + b);
        fclose(f);
      },
      10, 20);

  EXPECT_GT(proc.Pid(), 0);

  int status;
  waitpid(proc.Pid(), &status, 0);
  EXPECT_TRUE(WIFEXITED(status));

  FILE* f = fopen(tmp_file_.c_str(), "r");
  ASSERT_NE(f, nullptr);
  int result = 0;
  fscanf(f, "%d", &result);
  fclose(f);
  EXPECT_EQ(result, 30);
}

TEST_F(ProcessWrapTest, ChildExitCode) {
  ProcessWrap proc([]() { exit(42); });

  int status;
  waitpid(proc.Pid(), &status, 0);
  EXPECT_TRUE(WIFEXITED(status));
  EXPECT_EQ(WEXITSTATUS(status), 42);
}
