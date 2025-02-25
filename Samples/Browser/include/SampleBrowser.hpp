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
export module Ogre.Samples;

export import :Sample;
export import :SdkSample;
export import :DefaultSamplesPlugin;
export import :SamplePlugin;
export import :SampleContext;

export import Ogre.Components.Bites;
export import Ogre.Components.Overlay;
export import Ogre.Core;

export import <map>;
export import <memory>;

export
auto constexpr inline CAROUSEL_REDRAW_EPS = 0.001;

export
namespace OgreBites
{
    /*=============================================================================
      | The OGRE Sample Browser. Features a menu accessible from all samples,
      | dynamic configuration, resource reloading, node labeling, and more.
      =============================================================================*/
    class SampleBrowser : public SampleContext, public TrayListener
    {
        using PluginMap = std::map<Ogre::String, ::std::unique_ptr<SamplePlugin>>;
        PluginMap mPluginNameMap;                      // A structure to map plugin names to class types
    public:

    SampleBrowser(bool nograb = false, int startSampleIndex = -1)
    : SampleContext("OGRE Sample Browser"), mGrabInput(!nograb)
        {
            mIsShuttingDown = false;
            mTrayMgr = nullptr;
            mLastViewCategory = 0;
            mLastViewTitle = 0;
            mLastSampleIndex = -1;
            mStartSampleIndex = startSampleIndex;
            mCategoryMenu = nullptr;
            mSampleMenu = nullptr;
            mSampleSlider = nullptr;
            mTitleLabel = nullptr;
            mDescBox = nullptr;
            mRendererMenu = nullptr;
            mCarouselPlace = 0.0f;
        }

        void loadStartUpSample() override
        {
            if (mStartSampleIndex != -1)
            {
                runSampleByIndex(mStartSampleIndex);
                mStartSampleIndex = -1;
            }
        }

        virtual void runSampleByIndex(int idx)
        {
            runSample(::std::any_cast<Sample*>(mThumbs[idx]->getUserObjectBindings().getUserAny()));
        }

        /*-----------------------------------------------------------------------------
          | Extends runSample to handle creation and destruction of dummy scene.
          -----------------------------------------------------------------------------*/
        void runSample(Sample* s) override
        {
            if (mCurrentSample)  // sample quitting
            {
                mCurrentSample->_shutdown();
                mCurrentSample = nullptr;
                mSamplePaused = false;     // don't pause next sample

                // create dummy scene and modify controls
                createDummyScene();
                mTrayMgr->showBackdrop("SdkTrays/Bands");
                mTrayMgr->showAll();
                ((Button*)mTrayMgr->getWidget("StartStop"))->setCaption("Start Sample");
            }

            if (s)  // sample starting
            {
                // destroy dummy scene and modify controls
                ((Button*)mTrayMgr->getWidget("StartStop"))->setCaption("Stop Sample");
                mTrayMgr->showBackdrop("SdkTrays/Shade");
                mTrayMgr->hideAll();
                destroyDummyScene();

                try
                {
                    if(dynamic_cast<SdkSample*>(s))
                        s->_setupTrays(mWindow);
                    SampleContext::runSample(s);
                }
                catch (Ogre::Exception& e)   // if failed to start, show error and fall back to menu
                {
                    destroyDummyScene();

                    s->_shutdown();

                    createDummyScene();
                    mTrayMgr->showBackdrop("SdkTrays/Bands");
                    mTrayMgr->showAll();
                    ((Button*)mTrayMgr->getWidget("StartStop"))->setCaption("Start Sample");

                    mTrayMgr->showOkDialog("Error!", ::std::format("{}\nSource: {}", e.getDescription() , e.getSource()));
                }
            }
        }

        /// catch any exceptions that might drop out of event handlers implemented by Samples
        auto frameStarted(const Ogre::FrameEvent& evt) -> bool override
        {
            try
            {
                return SampleContext::frameStarted(evt);
            }
            catch (Ogre::Exception& e)   // show error and fall back to menu
            {
                runSample(nullptr);
                mTrayMgr->showOkDialog("Error!", ::std::format("{}\nSource: {}", e.getDescription() , e.getSource()));
            }

            return true;
        }

        /*-----------------------------------------------------------------------------
          | Extends frameRenderingQueued to update tray manager and carousel.
          -----------------------------------------------------------------------------*/
        auto frameRenderingQueued(const Ogre::FrameEvent& evt) -> bool override
        {
            // don't do all these calculations when sample's running or when in configuration screen or when no samples loaded
            if (!mLoadedSamples.empty() && mTitleLabel->getTrayLocation() != TrayLocation::NONE && (!mCurrentSample || mSamplePaused))
            {
                // makes the carousel spin smoothly toward its right position
                float carouselOffset = mSampleMenu->getSelectionIndex() - mCarouselPlace;
                if (std::abs(carouselOffset) <= CAROUSEL_REDRAW_EPS) mCarouselPlace = mSampleMenu->getSelectionIndex();
                else mCarouselPlace += carouselOffset * Ogre::Math::Clamp<float>(evt.timeSinceLastFrame * 15.0, -1.0, 1.0);

                // update the thumbnail positions based on carousel state
                for (int i = 0; i < (int)mThumbs.size(); i++)
                {
                    if(carouselOffset == 0) break;

                    Ogre::Real thumbOffset = mCarouselPlace - i;
                    Ogre::Real phase = (thumbOffset / 2.0) - 2.8;

                    if (thumbOffset < -5 || thumbOffset > 4)    // prevent thumbnails from wrapping around in a circle
                    {
                        mThumbs[i]->hide();
                        continue;
                    }
                    else mThumbs[i]->show();

                    Ogre::Real left = Ogre::Math::Cos(phase) * 200.0;
                    Ogre::Real top = Ogre::Math::Sin(phase) * 200.0;
                    Ogre::Real scale = 1.0 / Ogre::Math::Pow((Ogre::Math::Abs(thumbOffset) + 1.0), 0.75);

                    auto frame =
                        dynamic_cast<Ogre::BorderPanelOverlayElement*>(mThumbs[i]->getChildren().begin()->second);

                    mThumbs[i]->setDimensions(128.0 * scale, 96.0 * scale);
                    frame->setDimensions(mThumbs[i]->getWidth() + 16.0, mThumbs[i]->getHeight() + 16.0);
                    mThumbs[i]->setPosition((int)(left - 80.0 - (mThumbs[i]->getWidth() / 2.0)),
                                            (int)(top - 5.0 - (mThumbs[i]->getHeight() / 2.0)));
                    frame->setMaterial(nullptr); // dont draw inner region
                    if (i == mSampleMenu->getSelectionIndex()) frame->setBorderMaterialName("SdkTrays/Frame/Over");
                    else frame->setBorderMaterialName("SdkTrays/Frame");
                }
            }

            mTrayMgr->frameRendered(evt);

            return SampleContext::frameRenderingQueued(evt);
        }

        /*-----------------------------------------------------------------------------
          | Handles confirmation dialog responses.
          -----------------------------------------------------------------------------*/
        void yesNoDialogClosed(std::string_view question, bool yesHit) override
        {
            if (question.substr(0, 14) == "This will stop" && yesHit)   // confirm unloading of samples
            {
                runSample(nullptr);
                buttonHit((Button*)mTrayMgr->getWidget("UnloadReload"));
            }
        }

        /*-----------------------------------------------------------------------------
          | Handles button widget events.
          -----------------------------------------------------------------------------*/
        void buttonHit(Button* b) override
        {
            if (b->getName() == "StartStop")   // start or stop sample
            {
                if (b->getCaption() == "Start Sample")
                {
                    if (mLoadedSamples.empty()) {
                        mTrayMgr->showOkDialog("Error!", "No sample selected!");
                    } else {
                        // use the sample pointer we stored inside the thumbnail
                        Ogre::Renderable* r = mThumbs[mSampleMenu->getSelectionIndex()];
                        runSample(::std::any_cast<Sample*>(r->getUserObjectBindings().getUserAny()));
                    }
                } else
                    runSample(nullptr);
            }
            else if (b->getName() == "UnloadReload")   // unload or reload sample plugins and update controls
            {
                if (b->getCaption() == "Unload Samples")
                {
                    if (mCurrentSample) mTrayMgr->showYesNoDialog("Warning!", "This will stop the current sample. Unload anyway?");
                    else
                    {
                        // save off current view and try to restore it on the next reload
                        mLastViewTitle = mSampleMenu->getSelectionIndex();
                        mLastViewCategory = mCategoryMenu->getSelectionIndex();

                        unloadSamples();
                        populateSampleMenus();
                        b->setCaption("Reload Samples");
                    }
                }
                else
                {
                    loadSamples();
                    populateSampleMenus();
                    if (!mLoadedSamples.empty()) b->setCaption("Unload Samples");

                    try  // attempt to restore the last view before unloading samples
                    {
                        mCategoryMenu->selectItem(mLastViewCategory);
                        mSampleMenu->selectItem(mLastViewTitle);
                    }
                    catch (Ogre::Exception&) {}
                }
            }
            else if (b->getName() == "Configure")   // enter configuration screen
            {
                mTrayMgr->removeWidgetFromTray("StartStop");
                mTrayMgr->removeWidgetFromTray("Configure");

                mTrayMgr->removeWidgetFromTray("UnloadReload");
                mTrayMgr->removeWidgetFromTray("Quit");
                mTrayMgr->moveWidgetToTray("Apply", TrayLocation::RIGHT);

                mTrayMgr->moveWidgetToTray("Back", TrayLocation::RIGHT);

                for (auto & mThumb : mThumbs)
                {
                    mThumb->hide();
                }

                while (mTrayMgr->getTrayContainer(TrayLocation::CENTER)->isVisible())
                {
                    mTrayMgr->removeWidgetFromTray(TrayLocation::CENTER, 0);
                }

                while (mTrayMgr->getTrayContainer(TrayLocation::LEFT)->isVisible())
                {
                    mTrayMgr->removeWidgetFromTray(TrayLocation::LEFT, 0);
                }

                mTrayMgr->moveWidgetToTray("ConfigLabel", TrayLocation::LEFT);
                mTrayMgr->moveWidgetToTray(mRendererMenu, TrayLocation::LEFT);
                mTrayMgr->moveWidgetToTray("ConfigSeparator", TrayLocation::LEFT);

                mRendererMenu->selectItem(mRoot->getRenderSystem()->getName());

                windowResized(mWindow);
            }
            else if (b->getName() == "Back")   // leave configuration screen
            {
                while (mTrayMgr->getWidgets(mRendererMenu->getTrayLocation()).size() > 3)
                {
                    mTrayMgr->destroyWidget(mRendererMenu->getTrayLocation(), 3);
                }

                while (!mTrayMgr->getWidgets(TrayLocation::NONE).empty())
                {
                    mTrayMgr->moveWidgetToTray(TrayLocation::NONE, 0, TrayLocation::LEFT);
                }

                mTrayMgr->removeWidgetFromTray("Apply");
                mTrayMgr->removeWidgetFromTray("Back");
                mTrayMgr->removeWidgetFromTray("ConfigLabel");
                mTrayMgr->removeWidgetFromTray(mRendererMenu);
                mTrayMgr->removeWidgetFromTray("ConfigSeparator");

                mTrayMgr->moveWidgetToTray("StartStop", TrayLocation::RIGHT);

                mTrayMgr->moveWidgetToTray("UnloadReload", TrayLocation::RIGHT);

                mTrayMgr->moveWidgetToTray("Configure", TrayLocation::RIGHT);

                mTrayMgr->moveWidgetToTray("Quit", TrayLocation::RIGHT);

                mCarouselPlace += CAROUSEL_REDRAW_EPS;  // force redraw
                windowResized(mWindow);
            }
            else if (b->getName() == "Apply")   // apply any changes made in the configuration screen
            {
                bool reset = false;

                auto options =
                    mRoot->getRenderSystemByName(mRendererMenu->getSelectedItem())->getConfigOptions();

                Ogre::NameValuePairList newOptions;

                // collect new settings and decide if a reset is needed

                if (mRendererMenu->getSelectedItem() != mRoot->getRenderSystem()->getName()) {
                    reset = true;
                }

                for (unsigned int i = 3; i < mTrayMgr->getWidgets(mRendererMenu->getTrayLocation()).size(); i++)
                {
                    auto* menu = (SelectMenu*)mTrayMgr->getWidgets(mRendererMenu->getTrayLocation())[i];
                    if (menu->getSelectedItem() != options[menu->getCaption()].currentValue) reset = true;
                    newOptions[menu->getCaption()] = menu->getSelectedItem();
                }

                // reset with new settings if necessary
                if (reset) reconfigure(mRendererMenu->getSelectedItem(), newOptions);
            }
            else
            {
                mRoot->queueEndRendering();   // exit browser
            }
        }

        /*-----------------------------------------------------------------------------
          | Handles menu item selection changes.
          -----------------------------------------------------------------------------*/
        void itemSelected(SelectMenu* menu) override
        {
            if (menu == mCategoryMenu)      // category changed, so update the sample menu, carousel, and slider
            {
                for (auto & mThumb : mThumbs)    // destroy all thumbnails in carousel
                {
                    Ogre::MaterialManager::getSingleton().remove(mThumb->getName(), "Essential");
                    Widget::nukeOverlayElement(mThumb);
                }
                mThumbs.clear();

                Ogre::OverlayManager& om = Ogre::OverlayManager::getSingleton();
                Ogre::String selectedCategory;

                if (menu->getSelectionIndex() != -1) selectedCategory = menu->getSelectedItem();
                else
                {
                    mTitleLabel->setCaption("");
                    mDescBox->setText("");
                }

                bool all = selectedCategory == "All";
                Ogre::StringVector sampleTitles;
                Ogre::MaterialPtr templateMat = Ogre::MaterialManager::getSingleton().getByName("SdkTrays/SampleThumbnail");

                // populate the sample menu and carousel with filtered samples
                for (auto const& sample : mLoadedSamples)
                {
                    Ogre::NameValuePairList& info = sample->getInfo();

                    if (all || info["Category"] == selectedCategory)
                    {
                        Ogre::String name = ::std::format("SdkTrays/SampleThumb{}", sampleTitles.size() + 1);

                        // clone a new material for sample thumbnail
                        Ogre::MaterialPtr newMat = templateMat->clone(name);

                        Ogre::TextureUnitState* tus = newMat->getTechnique(0)->getPass(0)->getTextureUnitState(0);
                        tus->setTextureName(info["Thumbnail"]);

                        // create sample thumbnail overlay
                        auto bp = dynamic_cast<Ogre::PanelOverlayElement*>(
                            om.createOverlayElementFromTemplate("SdkTrays/Picture", "", name));
                        bp->setHorizontalAlignment(Ogre::GuiHorizontalAlignment::RIGHT);
                        bp->setVerticalAlignment(Ogre::GuiVerticalAlignment::CENTER);
                        bp->setMaterialName(name);
                        bp->getUserObjectBindings().setUserAny(sample);
                        mTrayMgr->getTraysLayer()->add2D(bp);

                        // add sample thumbnail and title
                        mThumbs.push_back(bp);
                        sampleTitles.push_back(sample->getInfo()["Title"]);
                    }
                }

                mCarouselPlace = CAROUSEL_REDRAW_EPS;  // reset carousel

                mSampleMenu->setItems(sampleTitles);
                if (mSampleMenu->getNumItems() != 0) itemSelected(mSampleMenu);

                mSampleSlider->setRange(1, static_cast<Ogre::Real>(sampleTitles.size()), static_cast<Ogre::Real>(sampleTitles.size()));
            }
            else if (menu == mSampleMenu)    // sample changed, so update slider, label and description
            {
                if (mSampleSlider->getValue() != menu->getSelectionIndex() + 1)
                    mSampleSlider->setValue(menu->getSelectionIndex() + 1);

                Ogre::Renderable* r = mThumbs[mSampleMenu->getSelectionIndex()];
                auto* s = ::std::any_cast<Sample*>(r->getUserObjectBindings().getUserAny());
                mTitleLabel->setCaption(menu->getSelectedItem());
                mDescBox->setText(::std::format("Category: {}\nDescription: {}", s->getInfo()["Category"], s->getInfo()["Description"]));

                if (mCurrentSample != s) ((Button*)mTrayMgr->getWidget("StartStop"))->setCaption("Start Sample");
                else ((Button*)mTrayMgr->getWidget("StartStop"))->setCaption("Stop Sample");
            }
            else if (menu == mRendererMenu)    // renderer selected, so update all settings
            {
                while (mTrayMgr->getWidgets(mRendererMenu->getTrayLocation()).size() > 3)
                {
                    mTrayMgr->destroyWidget(mRendererMenu->getTrayLocation(), 3);
                }

                auto options = mRoot->getRenderSystemByName(menu->getSelectedItem())->getConfigOptions();

                unsigned int i = 0;

                // create all the config option select menus
                for (auto & option : options)
                {
                    i++;
                    SelectMenu* optionMenu = mTrayMgr->createLongSelectMenu
                        (TrayLocation::LEFT, ::std::format("ConfigOption{}", i), option.first, 450, 240, 10);
                    optionMenu->setItems(option.second.possibleValues);

                    // if the current config value is not in the menu, add it
                    if(optionMenu->containsItem(option.second.currentValue) == false)
                    {
                        optionMenu->addItem(option.second.currentValue);
                    }

                    optionMenu->selectItem(option.second.currentValue);
                }

                windowResized(mWindow);
            }
        }

        /*-----------------------------------------------------------------------------
          | Handles sample slider changes.
          -----------------------------------------------------------------------------*/
        void sliderMoved(Slider* slider) override
        {
            // format the caption to be fraction style
            Ogre::String denom = ::std::format("{}/{}", slider->getValueCaption(), mSampleMenu->getNumItems());
            slider->setValueCaption(denom);

            // tell the sample menu to change if it hasn't already
            if (mSampleMenu->getSelectionIndex() != -1 && mSampleMenu->getSelectionIndex() != slider->getValue() - 1)
                mSampleMenu->selectItem(slider->getValue() - 1);
        }

        /*-----------------------------------------------------------------------------
          | Handles keypresses.
          -----------------------------------------------------------------------------*/
        auto keyPressed(const KeyDownEvent& evt) noexcept -> bool override
        {
            if (mTrayMgr->isDialogVisible()) return true;  // ignore keypresses when dialog is showing

            Keycode key = evt.keysym.sym;

            if (key == SDLK_ESCAPE)
            {
                if (mTitleLabel->getTrayLocation() != TrayLocation::NONE)
                {
                    // if we're in the main screen and a sample's running, toggle sample pause state
                    if (mCurrentSample)
                    {
                        if (mSamplePaused)
                        {
                            mTrayMgr->hideAll();
                            unpauseCurrentSample();
                        }
                        else
                        {
                            pauseCurrentSample();
                            mTrayMgr->showAll();
                        }
                    }
                }
                else buttonHit((Button*)mTrayMgr->getWidget("Back"));  // if we're in config, just go back
            }
            else if ((key == SDLK_UP || key == SDLK_DOWN) && mTitleLabel->getTrayLocation() != TrayLocation::NONE)
            {
                // if we're in the main screen, use the up and down arrow keys to cycle through samples
                int newIndex = mSampleMenu->getSelectionIndex() + (key == SDLK_UP ? -1 : 1);
                mSampleMenu->selectItem(Ogre::Math::Clamp<size_t>(newIndex, 0, mSampleMenu->getNumItems() - 1));
            }
            else if (key == SDLK_RETURN)   // start or stop sample
            {
                if (!mLoadedSamples.empty() && (mSamplePaused || mCurrentSample == nullptr))
                {
                    Ogre::Renderable* r = mThumbs[mSampleMenu->getSelectionIndex()];
                    auto* newSample = ::std::any_cast<Sample*>(r->getUserObjectBindings().getUserAny());
                    runSample(newSample == mCurrentSample ? nullptr : newSample);
                }
            }
            else if(key == SDLK_F9)   // toggle full screen
            {
                // Make sure we use the window size as originally requested, NOT the
                // current window size (which may have altered to fit desktop)
                auto desc = mRoot->getRenderSystem()->getRenderWindowDescription();
                mWindow->setFullscreen(!mWindow->isFullScreen(), desc.width, desc.height);
            }
            else if(key == SDLK_F11 || key == SDLK_F12) // Decrease and increase FSAA level on the fly
            {
                // current FSAA                0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16
                unsigned decreasedFSAA[17] = { 0, 0, 1, 2, 2, 4, 4, 4, 4, 8, 8, 8, 8, 8, 8, 8, 8 };
                unsigned increasedFSAA[17] = { 2, 2, 4, 4, 8, 8, 8, 8,16,16,16,16,16,16,16,16, 0, };
                unsigned FSAA = std::min(mWindow->getFSAA(), 16U);
                unsigned newFSAA = (key == SDLK_F12) ? increasedFSAA[FSAA] : decreasedFSAA[FSAA];
                if(newFSAA != 0)
                    mWindow->setFSAA(newFSAA, mWindow->getFSAAHint());
            }

            return SampleContext::keyPressed(evt);
        }

        /*-----------------------------------------------------------------------------
          | Extends pointerPressed to inject mouse press into tray manager, and to check
          | for thumbnail clicks, just because we can.
          -----------------------------------------------------------------------------*/
        auto mousePressed(const MouseButtonDownEvent& evt) noexcept -> bool override
        {
            if (mTitleLabel->getTrayLocation() != TrayLocation::NONE)
            {
                for (unsigned int i = 0; i < mThumbs.size(); i++)
                {
                    if (mThumbs[i]->isVisible() && Widget::isCursorOver(mThumbs[i],
                        Ogre::Vector2{mTrayMgr->getCursorContainer()->getLeft(), mTrayMgr->getCursorContainer()->getTop()}, 0))
                    {
                        mSampleMenu->selectItem(i);
                        return true;
                    }
                }
            }

            if (isCurrentSamplePaused()) return mTrayMgr->mousePressed(evt);

            return SampleContext::mousePressed(evt);
        }

        // convert and redirect
        auto touchPressed(const TouchFingerDownEvent& evt) noexcept -> bool override {
            MouseButtonDownEvent e;
            e.button = ButtonType::LEFT;
            return mousePressed(e);
        }

        auto buttonPressed(const ButtonDownEvent& evt) noexcept -> bool override
        {
            KeyDownEvent e;
            e.keysym.sym = 0;
            switch (evt.button)
            {
            case 0:
                e.keysym.sym = SDLK_RETURN;
                break;
            case 1:
                e.keysym.sym = SDLK_ESCAPE;
                break;
            case 11:
                e.keysym.sym = SDLK_UP;
                break;
            case 12:
                e.keysym.sym = SDLK_DOWN;
                break;
            }
            return keyPressed(e);
        }

        /*-----------------------------------------------------------------------------
          | Extends pointerReleased to inject mouse release into tray manager.
          -----------------------------------------------------------------------------*/
        auto mouseReleased(const MouseButtonUpEvent& evt) noexcept -> bool override
         {
            if (isCurrentSamplePaused()) return mTrayMgr->mouseReleased(evt);

            return SampleContext::mouseReleased(evt);
        }

        // convert and redirect
        auto touchReleased(const TouchFingerUpEvent& evt) noexcept -> bool override {
            MouseButtonUpEvent e;
            e.button = ButtonType::LEFT;
            return mouseReleased(e);
        }

        /*-----------------------------------------------------------------------------
          | Extends pointerMoved to inject mouse position into tray manager, and checks
          | for mouse wheel movements to slide the carousel, because we can.
          -----------------------------------------------------------------------------*/
        auto mouseMoved(const MouseMotionEvent& evt) noexcept -> bool override
        {
            if (isCurrentSamplePaused()) return mTrayMgr->mouseMoved(evt);

            return SampleContext::mouseMoved(evt);
        }

        // convert and redirect
        auto touchMoved(const TouchFingerMotionEvent& evt) noexcept -> bool override {
            MouseMotionEvent e;
            e.x = evt.x * mWindow->getWidth();
            e.y = evt.y * mWindow->getHeight();
            e.xrel = evt.dx * mWindow->getWidth();
            e.yrel = evt.dy * mWindow->getHeight();
            return mouseMoved(e);
        }

        //TODO: Handle iOS and Android.
        /** Mouse wheel scrolls the sample list.
         */
        auto mouseWheelRolled(const MouseWheelEvent& evt) noexcept -> bool override
        {
            if(mTrayMgr->mouseWheelRolled(evt))
                return true;

            if (isCurrentSamplePaused() && mTitleLabel->getTrayLocation() != TrayLocation::NONE &&
                mSampleMenu->getNumItems() != 0)
            {
                int newIndex = mSampleMenu->getSelectionIndex() - evt.y / Ogre::Math::Abs(evt.y);
                mSampleMenu->selectItem(Ogre::Math::Clamp<size_t>(newIndex, 0, mSampleMenu->getNumItems() - 1));
            }

            return SampleContext::mouseWheelRolled(evt);
        }

        /*-----------------------------------------------------------------------------
          | Extends windowResized to best fit menus on screen. We basically move the
          | menu tray to the left for higher resolutions and move it to the center
          | for lower resolutions.
          -----------------------------------------------------------------------------*/
        void windowResized(Ogre::RenderWindow* rw) override
        {
            if (!mTrayMgr) return;

            Ogre::OverlayContainer* center = mTrayMgr->getTrayContainer(TrayLocation::CENTER);
            Ogre::OverlayContainer* left = mTrayMgr->getTrayContainer(TrayLocation::LEFT);

            if (center->isVisible() && rw->getWidth() < 1280 - center->getWidth())
            {
                while (center->isVisible())
                {
                    mTrayMgr->moveWidgetToTray(mTrayMgr->getWidgets(TrayLocation::CENTER)[0], TrayLocation::LEFT);
                }
            }
            else if (left->isVisible() && rw->getWidth() >= 1280 - left->getWidth())
            {
                while (left->isVisible())
                {
                    mTrayMgr->moveWidgetToTray(mTrayMgr->getWidgets(TrayLocation::LEFT)[0], TrayLocation::CENTER);
                }
            }

            SampleContext::windowResized(rw);
        }

        /*-----------------------------------------------------------------------------
          | Extends setup to create dummy scene and tray interface.
          -----------------------------------------------------------------------------*/
        void setup() override
        {
            ApplicationContext::setup();
            mWindow = getRenderWindow();
            addInputListener(this);
            if(mGrabInput) setWindowGrab();
            else mTrayMgr->hideCursor();

            mPluginNameMap.emplace("DefaultSamples", new DefaultSamplesPlugin());

            Sample* startupSample = loadSamples();

            // create template material for sample thumbnails
            Ogre::MaterialPtr thumbMat = Ogre::MaterialManager::getSingleton().create("SdkTrays/SampleThumbnail", "Essential");
            thumbMat->setLightingEnabled(false);
            thumbMat->setDepthCheckEnabled(false);
            thumbMat->getTechnique(0)->getPass(0)->createTextureUnitState();

            setupWidgets();
            windowResized(mWindow);   // adjust menus for resolution

            // if this is our first time running, and there's a startup sample, run it
            if (startupSample && mFirstRun){
                runSample(startupSample);
            }
        }

    protected:
        /*-----------------------------------------------------------------------------
          | Overrides the default window title.
          -----------------------------------------------------------------------------*/
        auto createWindow(std::string_view name, uint32_t w, uint32_t h, Ogre::NameValuePairList miscParams) -> NativeWindowPair override
        {
            return ApplicationContext::createWindow(name, w, h, miscParams);
        }

        /*-----------------------------------------------------------------------------
          | Initialises only the browser's resources and those most commonly used
          | by samples. This way, additional special content can be initialised by
          | the samples that use them, so startup time is unaffected.
          -----------------------------------------------------------------------------*/
        void loadResources() override
        {
            Ogre::OverlayManager::getSingleton().setPixelRatio(getDisplayDPI()/96);

            Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup("Essential");
            mTrayMgr = new TrayManager("BrowserControls", getRenderWindow(), this);
            mTrayMgr->showBackdrop("SdkTrays/Bands");
            mTrayMgr->getTrayContainer(TrayLocation::NONE)->hide();

            // Ogre::TextureManager::getSingleton().setDefaultNumMipmaps(5);

            createDummyScene();

            Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
        }

        /*-----------------------------------------------------------------------------
          | Loads sample plugins from a configuration file.
          -----------------------------------------------------------------------------*/
        virtual auto loadSamples() -> Sample*
        {
            Sample* startupSample = nullptr;
            Ogre::StringVector unloadedSamplePlugins;

            Ogre::String startupSampleTitle;
            Ogre::StringVector sampleList;

            for(auto & it : mPluginNameMap)
            {
                sampleList.push_back(it.first);
            }

            // loop through all sample plugins...
            for (auto & i : sampleList)
            {
                SamplePlugin* sp = mPluginNameMap[i].get();

                // go through every sample in the plugin...
                for (auto const& sample : sp->getSamples())
                {
                    Ogre::NameValuePairList& info = sample->getInfo();   // acquire custom sample info

                    mLoadedSamples.insert(sample.get());                    // add sample only after ensuring title for sorting
                    mSampleCategories.insert(info["Category"]);   // add sample category

                    if (info["Title"] == startupSampleTitle) startupSample = sample.get();   // we found the startup sample
                }
            }

            if (!mLoadedSamples.empty()) mSampleCategories.insert("All");   // insert a category for all samples

            if (!unloadedSamplePlugins.empty())   // show error message summarising missing or invalid plugins
            {
                Ogre::String message = "These requested sample plugins were either missing, corrupt or invalid:";

                for (auto & unloadedSamplePlugin : unloadedSamplePlugins)
                {
                    message += ::std::format("\n- {}", unloadedSamplePlugin);
                }

                mTrayMgr->showOkDialog("Error!", message);
            }

            return startupSample;
        }

        /*-----------------------------------------------------------------------------
          | Unloads any loaded sample plugins.
          -----------------------------------------------------------------------------*/
        virtual void unloadSamples()
        {
            mLoadedSamples.clear();
            mLoadedSamplePlugins.clear();
            mSampleCategories.clear();
        }

        /*-----------------------------------------------------------------------------
          | Sets up main page for browsing samples.
          -----------------------------------------------------------------------------*/
        virtual void setupWidgets()
        {
            mTrayMgr->destroyAllWidgets();

            // create main navigation tray
            mTrayMgr->showLogo(TrayLocation::RIGHT);
            mTrayMgr->createSeparator(TrayLocation::RIGHT, "LogoSep");
            mTrayMgr->createButton(TrayLocation::RIGHT, "StartStop", "Start Sample", 120);

            mTrayMgr->createButton(TrayLocation::RIGHT, "UnloadReload", mLoadedSamples.empty() ? "Reload Samples" : "Unload Samples");

            mTrayMgr->createButton(TrayLocation::RIGHT, "Configure", "Configure");

            mTrayMgr->createButton(TrayLocation::RIGHT, "Quit", "Quit");

            // create sample viewing controls
            float infoWidth = 250;

            mTitleLabel = mTrayMgr->createLabel(TrayLocation::LEFT, "SampleTitle", "");
            mDescBox = mTrayMgr->createTextBox(TrayLocation::LEFT, "SampleInfo", "Sample Info", infoWidth, 208);
            mCategoryMenu = mTrayMgr->createThickSelectMenu(TrayLocation::LEFT, "CategoryMenu", "Select Category", infoWidth, 10);
            mSampleMenu = mTrayMgr->createThickSelectMenu(TrayLocation::LEFT, "SampleMenu", "Select Sample", infoWidth, 10);
            mSampleSlider = mTrayMgr->createThickSlider(TrayLocation::LEFT, "SampleSlider", "Slide Samples", infoWidth, 80, 0, 0, 0);

            /* Sliders do not notify their listeners on creation, so we manually call the callback here
               to format the slider value correctly. */
            sliderMoved(mSampleSlider);

            // create configuration screen button tray
            mTrayMgr->createButton(TrayLocation::NONE, "Apply", "Apply Changes");
            mTrayMgr->createButton(TrayLocation::NONE, "Back", "Go Back");

            // create configuration screen label and renderer menu
            mTrayMgr->createLabel(TrayLocation::NONE, "ConfigLabel", "Configuration");
            mRendererMenu = mTrayMgr->createLongSelectMenu(TrayLocation::NONE, "RendererMenu", "Render System", 450, 240, 10);
            mTrayMgr->createSeparator(TrayLocation::NONE, "ConfigSeparator");

            // populate render system names
            Ogre::StringVector rsNames;
            Ogre::RenderSystemList rsList = mRoot->getAvailableRenderers();
            for (auto & i : rsList)
            {
                rsNames.emplace_back(i->getName());
            }
            mRendererMenu->setItems(rsNames);

            populateSampleMenus();
        }

        /*-----------------------------------------------------------------------------
          | Populates home menus with loaded samples.
          -----------------------------------------------------------------------------*/
        virtual void populateSampleMenus()
        {
            Ogre::StringVector categories;
            for (const auto & mSampleCategorie : mSampleCategories)
                categories.push_back(mSampleCategorie);

            mCategoryMenu->setItems(categories);
            if (mCategoryMenu->getNumItems() != 0)
                mCategoryMenu->selectItem(0);
            else
                itemSelected(mCategoryMenu);   // if there are no items, we can't select one, so manually invoke callback

            mCarouselPlace = CAROUSEL_REDRAW_EPS; // force redraw
        }

        /*-----------------------------------------------------------------------------
          | Overrides to recover by last sample's index instead.
          -----------------------------------------------------------------------------*/
        void recoverLastSample() override
        {
            // restore the view while we're at it too
            mCategoryMenu->selectItem(mLastViewCategory);
            mSampleMenu->selectItem(mLastViewTitle);

            if (mLastSampleIndex != -1)
            {
                int index = -1;
                for (auto const& sample : mLoadedSamples)
                {
                    index++;
                    if (index == mLastSampleIndex)
                    {
                        runSample(sample);
                        sample->restoreState(mLastSampleState);
                        mLastSample = nullptr;
                        mLastSampleIndex = -1;
                        mLastSampleState.clear();
                    }
                }

                pauseCurrentSample();
                mTrayMgr->showAll();
            }

            buttonHit((Button*)mTrayMgr->getWidget("Configure"));
        }

        /*-----------------------------------------------------------------------------
          | Extends reconfigure to save the view and the index of last sample run.
          -----------------------------------------------------------------------------*/
        void reconfigure(std::string_view renderer, Ogre::NameValuePairList& options) override
        {
            mLastViewCategory = mCategoryMenu->getSelectionIndex();
            mLastViewTitle = mSampleMenu->getSelectionIndex();

            mLastSampleIndex = -1;
            unsigned int index = -1;
            for (auto const& sample : mLoadedSamples)
            {
                index++;
                if (sample == mCurrentSample)
                {
                    mLastSampleIndex = index;
                    break;
                }
            }

            SampleContext::reconfigure(renderer, options);
        }
    public:
        /*-----------------------------------------------------------------------------
          | Extends shutdown to destroy dummy scene and tray interface.
          -----------------------------------------------------------------------------*/
        void shutdown() override
        {
            if (mTrayMgr)
            {
                delete mTrayMgr;
                mTrayMgr = nullptr;
            }

            if (!mCurrentSample && mRoot->getRenderSystem() != nullptr) destroyDummyScene();

            SampleContext::shutdown();

            mCategoryMenu = nullptr;
            mSampleMenu = nullptr;
            mSampleSlider = nullptr;
            mTitleLabel = nullptr;
            mDescBox = nullptr;
            mRendererMenu = nullptr;
            mHiddenOverlays.clear();
            mThumbs.clear();
            mCarouselPlace = 0;
            mWindow = nullptr;

            unloadSamples();
        }
    protected:
        /*-----------------------------------------------------------------------------
          | Extend to temporarily hide a sample's overlays while in the pause menu.
          -----------------------------------------------------------------------------*/
        void pauseCurrentSample() override
        {
            SampleContext::pauseCurrentSample();

            Ogre::OverlayManager::OverlayMapIterator it = Ogre::OverlayManager::getSingleton().getOverlayIterator();
            mHiddenOverlays.clear();

            while (it.hasMoreElements())
            {
                Ogre::Overlay* o = it.getNext();
                if (o->isVisible())                  // later, we don't want to unhide the initially hidden overlays
                {
                    mHiddenOverlays.push_back(o);    // save off hidden overlays so we can unhide them later
                    o->hide();
                }
            }
        }

        /*-----------------------------------------------------------------------------
        | Extend to unhide all of sample's temporarily hidden overlays.
          -----------------------------------------------------------------------------*/
        void unpauseCurrentSample() override
        {
            SampleContext::unpauseCurrentSample();

            for (auto & mHiddenOverlay : mHiddenOverlays)
            {
                mHiddenOverlay->show();
            }

            mHiddenOverlays.clear();
        }

        TrayManager* mTrayMgr;                      // SDK tray interface
        Ogre::StringVector mLoadedSamplePlugins;       // loaded sample plugins
        std::set<Ogre::String> mSampleCategories;      // sample categories
        std::set<Sample*, Sample::Comparer> mLoadedSamples;                      // loaded samples
        SelectMenu* mCategoryMenu;                     // sample category select menu
        SelectMenu* mSampleMenu;                       // sample select menu
        Slider* mSampleSlider;                         // sample slider bar
        Label* mTitleLabel;                            // sample title label
        TextBox* mDescBox;                             // sample description box
        SelectMenu* mRendererMenu;                     // render system selection menu
        std::vector<Ogre::Overlay*> mHiddenOverlays;   // sample overlays hidden for pausing
        std::vector<Ogre::OverlayContainer*> mThumbs;  // sample thumbnails
        Ogre::Real mCarouselPlace;                     // current state of carousel
        int mLastViewTitle;                            // last sample title viewed
        int mLastViewCategory;                         // last sample category viewed
        int mLastSampleIndex;                          // index of last sample running
        int mStartSampleIndex;                         // directly starts the sample with the given index
    public:
        bool mIsShuttingDown;
        bool mGrabInput;
    };
}
