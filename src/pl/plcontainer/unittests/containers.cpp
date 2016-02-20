#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gtest/gtest.h"

extern "C" {
#include "containers.h"
}

TEST(Containers, CanCreate) {
    // stop the container after the test run
    PGconn_min *conn = start_container("plc_python");
    ASSERT_TRUE(conn != NULL);
}

TEST(Containers, CanFigureOutTheName) {
    const char *name = parse_container_name(
        "# container: plc_python\n"
        "return 'foobar'");
    ASSERT_STREQ(name, "plc_python");
}

TEST(Containers, CanFigureOutTheNameAndStart) {
    const char *name = parse_container_name(
        "# container: plc_python\n"
        "return 'foobar'");
    ASSERT_STREQ(name, "plc_python");
    // stop the container after the test run
    PGconn_min *conn = start_container(name);
    ASSERT_TRUE(conn != NULL);
}

TEST(Containers, CanFigureOutTheNameWhenThereIsANewline) {
    const char *name = parse_container_name(
        "\n"
        "# container: plc_python\n"
        "return 'foobar'");
    ASSERT_STREQ(name, "plc_python");
}

TEST(Containers, DiesWithNoContainerDeclaration) {
    ASSERT_DEATH(parse_container_name("return 'foobar'"),
                 "no container declaration");
}

TEST(Containers, DiesWithInvalidContainerDeclaration) {
    ASSERT_DEATH(parse_container_name("# container: plc_python"),
                 "no new line found");
}

TEST(Containers, DiesWithMissingColonInContainerDeclaration) {
    ASSERT_DEATH(parse_container_name("# container plc_python\n"),
                 "no colon found");
}

TEST(Containers, DiesWithEmptyImageName) {
    ASSERT_DEATH(parse_container_name("# container : \n"), "empty image name");
}

TEST(Containers, DiesWithEmptyString) {
    ASSERT_DEATH(parse_container_name(""), "empty string received");
}
