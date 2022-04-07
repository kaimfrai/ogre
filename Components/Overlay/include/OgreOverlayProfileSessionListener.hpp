/*
-----------------------------------------------------------------------------
This source file is part of OGRE
    (Object-oriented Graphics Rendering Engine)
For the latest info, see http://www.ogre3d.org/

Copyright (c) 2000-2014 Torus Knot Software Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/
export module Ogre.Components.Overlay:ProfileSessionListener;

export import Ogre.Core;

export import <list>;

export
namespace Ogre  {
    class Overlay;
    class OverlayContainer;
    class OverlayElement;

    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Overlays
    *  @{
    */

    /** Concrete impl. of the ProfileSessionListener which visualizes
        the profling results using overlays.
    */
    class OverlayProfileSessionListener : public ProfileSessionListener
    {
    public:
        enum DisplayMode
        {
            /// Display % frame usage on the overlay
            DISPLAY_PERCENTAGE,
            /// Display microseconds on the overlay
            DISPLAY_MICROSECONDS
        };

        OverlayProfileSessionListener();
        ~OverlayProfileSessionListener() override;

        /// @see ProfileSessionListener::initializeSession
        void initializeSession() override;

        /// @see ProfileSessionListener::finializeSession
        void finializeSession() override;

        /// @see ProfileSessionListener::displayResults
        void displayResults(const ProfileInstance& instance, ulong maxTotalFrameTime) override;

        /// @see ProfileSessionListener::changeEnableState
        void changeEnableState(bool enabled) override;

        /** Set the size of the profiler overlay, in pixels. */
        void setOverlayDimensions(Real width, Real height);

        /** Set the position of the profiler overlay, in pixels. */
        void setOverlayPosition(Real left, Real top);

        [[nodiscard]]
        auto getOverlayWidth() const -> Real;
        [[nodiscard]]
        auto getOverlayHeight() const -> Real;
        [[nodiscard]]
        auto getOverlayLeft() const -> Real;
        [[nodiscard]]
        auto getOverlayTop() const -> Real;

        /// Set the display mode for the overlay.
        void setDisplayMode(DisplayMode d) { mDisplayMode = d; }

        /// Get the display mode for the overlay.
        [[nodiscard]]
        auto getDisplayMode() const -> DisplayMode { return mDisplayMode; }

    private:
        using ProfileBarList = std::list<OverlayElement *>;

        /** Prints the profiling results of each frame 
        @remarks Recursive, for all the little children. */
        void displayResults(ProfileInstance* instance, ProfileBarList::const_iterator& bIter, ulong& maxTimeClocks, Real& newGuiHeight, int& profileCount);

        /** An internal function to create the container which will hold our display elements*/
        auto createContainer() -> OverlayContainer*;

        /** An internal function to create a text area */
        auto createTextArea(const String& name, Real width, Real height, Real top, Real left, 
                                    uint fontSize, const String& caption, bool show = true) -> OverlayElement*;

        /** An internal function to create a panel */
        auto createPanel(const String& name, Real width, Real height, Real top, Real left, 
                                const String& materialName, bool show = true) -> OverlayElement*;

        /// Holds the display bars for each profile results
        ProfileBarList mProfileBars;

        /// The overlay which contains our profiler results display
        Overlay* mOverlay{nullptr};

        /// The window that displays the profiler results
        OverlayContainer* mProfileGui{nullptr};

        /// The height of each bar
        Real mBarHeight{10};

        /// The height of the stats window
        Real mGuiHeight{25};

        /// The width of the stats window
        Real mGuiWidth{250};

        /// The horz position of the stats window
        Real mGuiLeft{0};

        /// The vertical position of the stats window
        Real mGuiTop{0};

        /// The size of the indent for each profile display bar
        Real mBarIndent{250};

        /// The width of the border between the profile window and each bar
        Real mGuiBorderWidth{10};

        /// The width of the min, avg, and max lines in a profile display
        Real mBarLineWidth{2};

        /// The distance between bars
        Real mBarSpacing{3};

        /// The max number of profiles we can display
        uint mMaxDisplayProfiles{50};

        /// How to display the overlay
        DisplayMode mDisplayMode{DISPLAY_MICROSECONDS};
    };
}
