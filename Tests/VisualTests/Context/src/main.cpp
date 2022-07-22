import Ogre.Tests.VisualTests.Context;

import <iostream>;

auto main(int argc, char *argv[]) -> int
{
    TestContext tc(argc, argv);

    try
    {
        tc.go();
    }
    catch (Ogre::Exception& e)
    {

        std::cerr << "An exception has occurred: " << e.getFullDescription() << std::endl;

        return -1;
    }

    return !tc.wasSuccessful();
}
