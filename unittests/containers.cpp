#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "gtest/gtest.h"

extern "C" {
#include "containers.h"
}

TEST(Containers, CanCreate) {
    // stop the container after the test run
    plcConn *conn = start_container("plc_python", 0);
    ASSERT_TRUE(conn != NULL);
}

TEST(Containers, CanFigureOutTheName) {
    int shared;
    const char *name = parse_container_meta(
        "# container: plc_python\n"
        "return 'foobar'", &shared);
    ASSERT_STREQ(name, "plc_python");
    ASSERT_TRUE(shared == 0);
}

TEST(Containers, CanFigureOutTheNameAndStart) {
    int shared;
    const char *name = parse_container_meta(
        "# container: plc_python\n"
        "return 'foobar'", &shared);
    ASSERT_STREQ(name, "plc_python");
    ASSERT_TRUE(shared == 0);
    // stop the container after the test run
    plcConn *conn = start_container(name, 0);
    ASSERT_TRUE(conn != NULL);
}

TEST(Containers, CanFigureOutTheNameWhenThereIsANewline) {
    int shared;
    const char *name = parse_container_meta(
        "\n"
        "# container: plc_python\n"
        "return 'foobar'", &shared);
    ASSERT_STREQ(name, "plc_python");
    ASSERT_TRUE(shared == 0);
}

TEST(Containers, CanFigureOutContainerIsShared) {
    int shared;
    const char *name = parse_container_meta(
        "\n"
        "# container: plc_python : shared \n"
        "return 'foobar'", &shared);
    ASSERT_STREQ(name, "plc_python");
    ASSERT_TRUE(shared == 1);
}

TEST(Containers, DiesWithNoContainerDeclaration) {
    int shared;
    ASSERT_DEATH(parse_container_meta("return 'foobar'", &shared),
                 "No container declaration found");
}

TEST(Containers, DiesWithNoContainerInContainerDeclaration) {
    int shared;
    ASSERT_DEATH(parse_container_meta("# foobar: plc_python\n\n", &shared),
                 "Container declaration should start with '#container:'");
}

TEST(Containers, DiesWithMissingColonInContainerDeclaration) {
    int shared;
    ASSERT_DEATH(parse_container_meta("# container plc_python\n", &shared),
                 "No colon found in container declaration");
}

TEST(Containers, DiesWithEmptyImageName) {
    int shared;
    ASSERT_DEATH(parse_container_meta("# container : \n", &shared),
                "Container name cannot be empty");
}

TEST(Containers, DiesWithEmptyString) {
    int shared;
    ASSERT_DEATH(parse_container_meta("", &shared),
                "No container declaration found");
}

TEST(Containers, DiesWithEmptySharing) {
    int shared;
    ASSERT_DEATH(parse_container_meta("# container : plc_python : \n", &shared),
                "Container sharing mode declaration is empty");
}

TEST(Containers, DiesWithBadSharing) {
    int shared;
    ASSERT_DEATH(parse_container_meta("# container : plc_python : nonshared \n", &shared),
                "Container sharing mode declaration can be only 'shared'");
}
