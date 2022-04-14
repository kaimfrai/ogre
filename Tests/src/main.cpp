#include <gtest/gtest.h>

import Ogre.Core;
import Ogre.Tests;

auto main(int argc, char *argv[]) -> int
{
    Ogre::LogManager logMgr{};
    logMgr.createLog("OgreTest.log", true, false);
    logMgr.setMinLogLevel(Ogre::LML_TRIVIAL);

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
