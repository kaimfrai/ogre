module;

#include <gtest/gtest.h>

module Ogre.Tests;

import Ogre.Core;

auto main(int argc, char *argv[]) -> int
{
    Ogre::LogManager logMgr{};
    logMgr.createLog("OgreTest.log", true, false);
    logMgr.setMinLogLevel(Ogre::LogMessageLevel::Trivial);

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
