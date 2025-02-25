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
module;

#include <cstddef>

export module Ogre.Components.Bites:Trays;

export import :Input;

export import Ogre.Components.Overlay;
export import Ogre.Core;

export import <format>;
export import <memory>;
export import <string>;
export import <vector>;

export
namespace OgreBites
{
    /** \addtogroup Optional
    *  @{
    */
    /** \addtogroup Bites
    *  @{
    */
    /** @defgroup Trays Trays
     * Simplistic GUI System build with Overlays
     * @{
     */
    enum class TrayLocation   /// enumerator values for widget tray anchoring locations
    {
        TOPLEFT,
        TOP,
        TOPRIGHT,
        LEFT,
        CENTER,
        RIGHT,
        BOTTOMLEFT,
        BOTTOM,
        BOTTOMRIGHT,
        NONE
    };

    enum class ButtonState   /// enumerator values for button states
    {
        UP,
        OVER,
        DOWN
    };

    // forward widget class declarations
    class Button;
    class SelectMenu;
    class Label;
    class Slider;
    class CheckBox;

    /**
    Listener class for responding to tray events.
    */
    class TrayListener
    {
    public:

        virtual ~TrayListener() = default;
        virtual void buttonHit(Button* button) {}
        virtual void itemSelected(SelectMenu* menu) {}
        virtual void labelHit(Label* label) {}
        virtual void sliderMoved(Slider* slider) {}
        virtual void checkBoxToggled(CheckBox* box) {}
        virtual void okDialogClosed(std::string_view message) {}
        virtual void yesNoDialogClosed(std::string_view question, bool yesHit) {}
    };

    /**
    Abstract base class for all widgets.
    */
    class Widget
    {
    public:
            
        Widget();

        virtual ~Widget() = default;

        void cleanup();

        /**
        Static utility method to recursively delete an overlay element plus
        all of its children from the system.
        */
        static void nukeOverlayElement(Ogre::OverlayElement* element);

        /**
        Static utility method to check if the cursor is over an overlay element.
        */
        static auto isCursorOver(Ogre::OverlayElement* element, const Ogre::Vector2& cursorPos, Ogre::Real voidBorder = 0) -> bool;

        /**
        Static utility method used to get the cursor's offset from the center
        of an overlay element in pixels.
        */
        static auto cursorOffset(Ogre::OverlayElement* element, const Ogre::Vector2& cursorPos) -> Ogre::Vector2;

        /**
        Static utility method used to get the width of a caption in a text area.
        */
        static auto getCaptionWidth(std::string_view caption, Ogre::TextAreaOverlayElement* area) -> Ogre::Real;

        /**
        Static utility method to cut off a string to fit in a text area.
        */
        static void fitCaptionToArea(std::string_view caption, Ogre::TextAreaOverlayElement* area, Ogre::Real maxWidth);

        auto getOverlayElement() noexcept -> Ogre::OverlayElement*
        {
            return mElement;
        }

        auto getName() noexcept -> std::string_view
        {
            return mElement->getName();
        }

        auto getTrayLocation() -> TrayLocation
        {
            return mTrayLoc;
        }

        void hide()
        {
            mElement->hide();
        }

        void show()
        {
            mElement->show();
        }

        auto isVisible() noexcept -> bool
        {
            return mElement->isVisible();
        }

        // callbacks

        virtual void _cursorPressed(const Ogre::Vector2& cursorPos) {}
        virtual void _cursorReleased(const Ogre::Vector2& cursorPos) {}
        virtual void _cursorMoved(const Ogre::Vector2& cursorPos, float wheelDelta) {}
        virtual void _focusLost() {}

        // internal methods used by TrayManager. do not call directly.

        void _assignToTray(TrayLocation trayLoc) { mTrayLoc = trayLoc; }
        void _assignListener(TrayListener* listener) { mListener = listener; }

    protected:

        Ogre::OverlayElement* mElement;
        TrayLocation mTrayLoc;
        TrayListener* mListener;
    };

    using WidgetList = std::vector<Widget *>;

    /**
    Basic button class.
    */
    class Button : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        Button(std::string_view name, std::string_view caption, Ogre::Real width);

        ~Button() override = default;

        auto getCaption() noexcept -> std::string_view
        {
            return mTextArea->getCaption();
        }

        void setCaption(std::string_view caption);

        auto getState() noexcept -> const ButtonState& { return mState; }

        void _cursorPressed(const Ogre::Vector2& cursorPos) override;

        void _cursorReleased(const Ogre::Vector2& cursorPos) override;

        void _cursorMoved(const Ogre::Vector2& cursorPos, float wheelDelta) override;

        void _focusLost() override;

    protected:

        void setState(const ButtonState& bs);

        ButtonState mState;
        Ogre::BorderPanelOverlayElement* mBP;
        Ogre::TextAreaOverlayElement* mTextArea;
        bool mFitToContents;
    };  

    /**
    Scrollable text box widget.
    */
    class TextBox : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        TextBox(std::string_view name, std::string_view caption, Ogre::Real width, Ogre::Real height);

        void setPadding(Ogre::Real padding);

        auto getPadding() -> Ogre::Real
        {
            return mPadding;
        }

        auto getCaption() noexcept -> std::string_view
        {
            return mCaptionTextArea->getCaption();
        }

        void setCaption(std::string_view caption)
        {
            mCaptionTextArea->setCaption(caption);
        }

        auto getText() noexcept -> std::string_view
        {
            return mText;
        }

        /**
        Sets text box content. Most of this method is for wordwrap.
        */
        void setText(std::string_view text);

        /**
        Sets text box content horizontal alignment.
        */
        void setTextAlignment(Ogre::TextAreaOverlayElement::Alignment ta);

        void clearText()
        {
            setText("");
        }

        void appendText(std::string_view text)
        {
            setText(std::format("{}{}", getText(), text));
        }

        /**
        Makes adjustments based on new padding, size, or alignment info.
        */
        void refitContents();

        /**
        Sets how far scrolled down the text is as a percentage.
        */
        void setScrollPercentage(Ogre::Real percentage);

        /**
        Gets how far scrolled down the text is as a percentage.
        */
        auto getScrollPercentage() -> Ogre::Real
        {
            return mScrollPercentage;
        }

        /**
        Gets how many lines of text can fit in this window.
        */
        auto getHeightInLines() noexcept -> unsigned int
        {
            return (unsigned int) ((mElement->getHeight() - 2 * mPadding - mCaptionBar->getHeight() + 5) / mTextArea->getCharHeight());
        }

        void _cursorPressed(const Ogre::Vector2& cursorPos) override;

        void _cursorReleased(const Ogre::Vector2& cursorPos) override
        {
            mDragging = false;
        }

        void _cursorMoved(const Ogre::Vector2& cursorPos, float wheelDelta) override;

        void _focusLost() override
        {
            mDragging = false;  // stop dragging if cursor was lost
        }

    protected:

        /**
        Decides which lines to show.
        */
        void filterLines();

        Ogre::TextAreaOverlayElement* mTextArea;
        Ogre::BorderPanelOverlayElement* mCaptionBar;
        Ogre::TextAreaOverlayElement* mCaptionTextArea;
        Ogre::BorderPanelOverlayElement* mScrollTrack;
        Ogre::PanelOverlayElement* mScrollHandle;
        Ogre::DisplayString mText;
        Ogre::StringVector mLines;
        Ogre::Real mPadding;
        bool mDragging;
        Ogre::Real mScrollPercentage;
        Ogre::Real mDragOffset;
        unsigned int mStartingLine;
    };

    /**
    Basic selection menu widget.
    */
    class SelectMenu : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        SelectMenu(std::string_view name, std::string_view caption, Ogre::Real width,
            Ogre::Real boxWidth, size_t maxItemsShown);
        void copyItemsFrom(SelectMenu* other);
        auto isExpanded() noexcept -> bool
        {
            return mExpanded;
        }

        auto getCaption() noexcept -> std::string_view
        {
            return mTextArea->getCaption();
        }

        void setCaption(std::string_view caption);

        auto getItems() noexcept -> const Ogre::StringVector&
        {
            return mItems;
        }

        auto getNumItems() -> size_t
        {
            return mItems.size();
        }

        void setItems(const Ogre::StringVector& items);

        void addItem(std::string_view item)
        {
            mItems.emplace_back(item);
            setItems(mItems);
        }

        void insertItem(size_t index, std::string_view item)
        {
            mItems.emplace(mItems.begin() + index, item);
            setItems(mItems);
        }

        void removeItem(std::string_view item);

        void removeItem(size_t index);

        void clearItems();

        void selectItem(size_t index, bool notifyListener = true);

        auto containsItem(std::string_view item) -> bool;

        void selectItem(std::string_view item, bool notifyListener = true);

        auto getSelectedItem() -> Ogre::DisplayString;

        auto getSelectionIndex() noexcept -> int
        {
            return mSelectionIndex;
        }

        void _cursorPressed(const Ogre::Vector2& cursorPos) override;

        void _cursorReleased(const Ogre::Vector2& cursorPos) override
        {
            mDragging = false;
        }

        void _cursorMoved(const Ogre::Vector2& cursorPos, float wheelDelta) override;

        void _focusLost() override
        {
            if (mExpandedBox->isVisible()) retract();
        }

    protected:

        /**
        Internal method - sets which item goes at the top of the expanded menu.
        */
        void setDisplayIndex(unsigned int index);

        /**
        Internal method - cleans up an expanded menu.
        */
        void retract();

        Ogre::BorderPanelOverlayElement* mSmallBox;
        Ogre::BorderPanelOverlayElement* mExpandedBox;
        Ogre::TextAreaOverlayElement* mTextArea;
        Ogre::TextAreaOverlayElement* mSmallTextArea;
        Ogre::BorderPanelOverlayElement* mScrollTrack;
        Ogre::PanelOverlayElement* mScrollHandle;
        std::vector<Ogre::BorderPanelOverlayElement*> mItemElements;
        size_t mMaxItemsShown;
        size_t mItemsShown;
        bool mCursorOver;
        bool mExpanded;
        bool mFitToContents;
        bool mDragging;
        Ogre::StringVector mItems;
        int mSelectionIndex;
        int mHighlightIndex{0};
        int mDisplayIndex{0};
        Ogre::Real mDragOffset{0.0f};
    };

    /**
    Basic label widget.
    */
    class Label : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        Label(std::string_view name, std::string_view caption, Ogre::Real width);

        auto getCaption() noexcept -> std::string_view
        {
            return mTextArea->getCaption();
        }

        void setCaption(std::string_view caption)
        {
            mTextArea->setCaption(caption);
        }

        void _cursorPressed(const Ogre::Vector2& cursorPos) override;

        auto _isFitToTray() -> bool
        {
            return mFitToTray;
        }

    protected:

        Ogre::TextAreaOverlayElement* mTextArea;
        bool mFitToTray;
    };

    /**
    Basic separator widget.
    */
    class Separator : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        Separator(std::string_view name, Ogre::Real width);

        auto _isFitToTray() -> bool
        {
            return mFitToTray;
        }

    protected:

        bool mFitToTray;
    };

    /**
    Basic slider widget.
    */
    class Slider : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        Slider(std::string_view name, std::string_view caption, Ogre::Real width, Ogre::Real trackWidth,
            Ogre::Real valueBoxWidth, Ogre::Real minValue, Ogre::Real maxValue, unsigned int snaps);

        /**
        Sets the minimum value, maximum value, and the number of snapping points.
        */
        void setRange(Ogre::Real minValue, Ogre::Real maxValue, unsigned int snaps, bool notifyListener = true);

        auto getValueCaption() noexcept -> std::string_view
        {
            return mValueTextArea->getCaption();
        }
        
        /**
        You can use this method to manually format how the value is displayed.
        */
        void setValueCaption(std::string_view caption)
        {
            mValueTextArea->setCaption(caption);
        }

        void setValue(Ogre::Real value, bool notifyListener = true);

        auto getValue() -> Ogre::Real
        {
            return mValue;
        }

        auto getCaption() noexcept -> std::string_view
        {
            return mTextArea->getCaption();
        }

        void setCaption(std::string_view caption);

        void _cursorPressed(const Ogre::Vector2& cursorPos) override;

        void _cursorReleased(const Ogre::Vector2& cursorPos) override;

        void _cursorMoved(const Ogre::Vector2& cursorPos, float wheelDelta) override;

        void _focusLost() override
        {
            mDragging = false;
        }

    protected:

        /**
        Internal method - given a percentage (from left to right), gets the
        value of the nearest marker.
        */
        auto getSnappedValue(Ogre::Real percentage) -> Ogre::Real
        {
            percentage = Ogre::Math::saturate(percentage);
            auto whichMarker = (unsigned int) (percentage * (mMaxValue - mMinValue) / mInterval + 0.5);
            return float(whichMarker) * mInterval + mMinValue;
        }

        Ogre::TextAreaOverlayElement* mTextArea;
        Ogre::TextAreaOverlayElement* mValueTextArea;
        Ogre::BorderPanelOverlayElement* mTrack;
        Ogre::PanelOverlayElement* mHandle;
        bool mDragging;
        bool mFitToContents;
        Ogre::Real mDragOffset{0.0f};
        Ogre::Real mValue{0.0f};
        Ogre::Real mMinValue{0.0f};
        Ogre::Real mMaxValue{0.0f};
        Ogre::Real mInterval{0.0f};
    };

    /**
    Basic parameters panel widget.
    */
    class ParamsPanel : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        ParamsPanel(std::string_view name, Ogre::Real width, unsigned int lines);

        void setAllParamNames(const Ogre::StringVector& paramNames);

        auto getAllParamNames() noexcept -> const Ogre::StringVector&
        {
            return mNames;
        }

        void setAllParamValues(const Ogre::StringVector& paramValues);

        void setParamValue(std::string_view paramName, std::string_view paramValue);

        void setParamValue(unsigned int index, std::string_view paramValue);

        auto getParamValue(std::string_view paramName) -> Ogre::DisplayString;

        auto getParamValue(unsigned int index) -> Ogre::DisplayString;

        auto getAllParamValues() noexcept -> const Ogre::StringVector&
        {
            return mValues;
        }

    protected:

        /**
        Internal method - updates text areas based on name and value lists.
        */
        void updateText();

        Ogre::TextAreaOverlayElement* mNamesArea;
        Ogre::TextAreaOverlayElement* mValuesArea;
        Ogre::StringVector mNames;
        Ogre::StringVector mValues;
    };

    /**
    Basic check box widget.
    */
    class CheckBox : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        CheckBox(std::string_view name, std::string_view caption, Ogre::Real width);

        auto getCaption() noexcept -> std::string_view
        {
            return mTextArea->getCaption();
        }

        void setCaption(std::string_view caption);

        auto isChecked() noexcept -> bool
        {
            return mX->isVisible();
        }

        void setChecked(bool checked, bool notifyListener = true);

        void toggle(bool notifyListener = true);

        void _cursorPressed(const Ogre::Vector2& cursorPos) override;

        void _cursorMoved(const Ogre::Vector2& cursorPos, float wheelDelta) override;

        void _focusLost() override;

    protected:

        Ogre::TextAreaOverlayElement* mTextArea;
        Ogre::BorderPanelOverlayElement* mSquare;
        Ogre::OverlayElement* mX;
        bool mFitToContents;
        bool mCursorOver;
    };

    /**
    Custom, decorative widget created from a template.
    */
    class DecorWidget : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        DecorWidget(std::string_view name, std::string_view templateName);
    };

    /**
    Basic progress bar widget.
    */
    class ProgressBar : public Widget
    {
    public:

        /// Do not instantiate any widgets directly. Use TrayManager.
        ProgressBar(std::string_view name, std::string_view caption, Ogre::Real width, Ogre::Real commentBoxWidth);

        /**
        Sets the progress as a percentage.
        */
        void setProgress(Ogre::Real progress);

        /**
        Gets the progress as a percentage.
        */
        auto getProgress() -> Ogre::Real
        {
            return mProgress;
        }

        auto getCaption() noexcept -> std::string_view
        {
            return mTextArea->getCaption();
        }

        void setCaption(std::string_view caption)
        {
            mTextArea->setCaption(caption);
        }

        auto getComment() noexcept -> std::string_view
        {
            return mCommentTextArea->getCaption();
        }

        void setComment(std::string_view comment)
        {
            mCommentTextArea->setCaption(comment);
        }


    protected:

        Ogre::TextAreaOverlayElement* mTextArea;
        Ogre::TextAreaOverlayElement* mCommentTextArea;
        Ogre::OverlayElement* mMeter;
        Ogre::OverlayElement* mFill;
        Ogre::Real mProgress{0.0f};
    };

    /**
    Main class to manage a cursor, backdrop, trays and widgets.
    */
    class TrayManager : public TrayListener, public Ogre::ResourceGroupListener, public InputListener
    {
    public:

        /**
        Creates backdrop, cursor, and trays.
        */
        TrayManager(std::string_view name, Ogre::RenderWindow* window, TrayListener* listener = nullptr);

        /**
        Destroys background, cursor, widgets, and trays.
        */
        ~TrayManager() override;

        /**
        Converts a 2D screen coordinate (in pixels) to a 3D ray into the scene.
        */
        static auto screenToScene(Ogre::Camera* cam, const Ogre::Vector2& pt) -> Ogre::Ray;

        /**
        Converts a 3D scene position to a 2D screen position (in relative screen size, 0.0-1.0).
        */
        static auto sceneToScreen(Ogre::Camera* cam, const Ogre::Vector3& pt) -> Ogre::Vector2;

        // these methods get the underlying overlays and overlay elements

        auto getTrayContainer(TrayLocation trayLoc) -> Ogre::OverlayContainer* { return mTrays[std::to_underlying(trayLoc)]; }
        auto getBackdropLayer() noexcept -> Ogre::Overlay* { return mBackdropLayer; }
        auto getTraysLayer() noexcept -> Ogre::Overlay* { return mTraysLayer; }
        auto getCursorLayer() noexcept -> Ogre::Overlay* { return mCursorLayer; }
        auto getBackdropContainer() noexcept -> Ogre::OverlayContainer* { return mBackdrop; }
        auto getCursorContainer() noexcept -> Ogre::OverlayContainer* { return mCursor; }
        auto getCursorImage() noexcept -> Ogre::OverlayElement* { return mCursor->getChild(::std::format("{}/CursorImage", mCursor->getName())); }

        void setListener(TrayListener* listener)
        {
            mListener = listener;
        }

        auto getListener() noexcept -> TrayListener*
        {
            return mListener;
        }

        void showAll();

        void hideAll();

        /**
        Displays specified material on backdrop, or the last material used if
        none specified. Good for pause menus like in the browser.
        */
        void showBackdrop(std::string_view materialName = Ogre::BLANKSTRING);

        void hideBackdrop()
        {
            mBackdropLayer->hide();
        }

        /**
        Displays specified material on cursor, or the last material used if
        none specified. Used to change cursor type.
        */
        void showCursor(std::string_view materialName = Ogre::BLANKSTRING);

        void hideCursor();

        /**
        Updates cursor position based on unbuffered mouse state. This is necessary
        because if the tray manager has been cut off from mouse events for a time,
        the cursor position will be out of date.
        */
        void refreshCursor();

        void showTrays();

        void hideTrays();

        auto isCursorVisible() noexcept -> bool { return mCursorLayer->isVisible(); }
        auto isBackdropVisible() noexcept -> bool { return mBackdropLayer->isVisible(); }
        auto areTraysVisible() -> bool { return mTraysLayer->isVisible(); }

        /**
        Sets horizontal alignment of a tray's contents.
        */
        void setTrayWidgetAlignment(TrayLocation trayLoc, Ogre::GuiHorizontalAlignment gha);

        // padding and spacing methods

        void setWidgetPadding(Ogre::Real padding);

        void setWidgetSpacing(Ogre::Real spacing);
        void setTrayPadding(Ogre::Real padding);

        [[nodiscard]] virtual auto getWidgetPadding() const noexcept -> Ogre::Real { return mWidgetPadding; }
        [[nodiscard]] virtual auto getWidgetSpacing() const noexcept -> Ogre::Real { return mWidgetSpacing; }
        [[nodiscard]] virtual auto getTrayPadding() const noexcept -> Ogre::Real { return mTrayPadding; }

        /**
        Fits trays to their contents and snaps them to their anchor locations.
        */
        virtual void adjustTrays();

        /**
        Returns a 3D ray into the scene that is directly underneath the cursor.
        */
        auto getCursorRay(Ogre::Camera* cam) -> Ogre::Ray;

        auto createButton(TrayLocation trayLoc, std::string_view name, std::string_view caption, Ogre::Real width = 0) -> Button*;

        auto createTextBox(TrayLocation trayLoc, std::string_view name, std::string_view caption,
            Ogre::Real width, Ogre::Real height) -> TextBox*;

        auto createThickSelectMenu(TrayLocation trayLoc, std::string_view name, std::string_view caption,
            Ogre::Real width, unsigned int maxItemsShown, const Ogre::StringVector& items = Ogre::StringVector()) -> SelectMenu*;

        auto createLongSelectMenu(TrayLocation trayLoc, std::string_view name, std::string_view caption,
            Ogre::Real width, Ogre::Real boxWidth, unsigned int maxItemsShown, const Ogre::StringVector& items = Ogre::StringVector()) -> SelectMenu*;

        auto createLongSelectMenu(TrayLocation trayLoc, std::string_view name, std::string_view caption,
            Ogre::Real boxWidth, unsigned int maxItemsShown, const Ogre::StringVector& items = Ogre::StringVector()) -> SelectMenu*;

        auto createLabel(TrayLocation trayLoc, std::string_view name, std::string_view caption, Ogre::Real width = 0) -> Label*;

        auto createSeparator(TrayLocation trayLoc, std::string_view name, Ogre::Real width = 0) -> Separator*;

        auto createThickSlider(TrayLocation trayLoc, std::string_view name, std::string_view caption,
            Ogre::Real width, Ogre::Real valueBoxWidth, Ogre::Real minValue, Ogre::Real maxValue, unsigned int snaps) -> Slider*;

        auto createLongSlider(TrayLocation trayLoc, std::string_view name, std::string_view caption, Ogre::Real width,
            Ogre::Real trackWidth, Ogre::Real valueBoxWidth, Ogre::Real minValue, Ogre::Real maxValue, unsigned int snaps) -> Slider*;

        auto createLongSlider(TrayLocation trayLoc, std::string_view name, std::string_view caption,
            Ogre::Real trackWidth, Ogre::Real valueBoxWidth, Ogre::Real minValue, Ogre::Real maxValue, unsigned int snaps) -> Slider*;

        auto createParamsPanel(TrayLocation trayLoc, std::string_view name, Ogre::Real width, unsigned int lines) -> ParamsPanel*;

        auto createParamsPanel(TrayLocation trayLoc, std::string_view name, Ogre::Real width,
            const Ogre::StringVector& paramNames) -> ParamsPanel*;

        auto createCheckBox(TrayLocation trayLoc, std::string_view name, std::string_view caption,
            Ogre::Real width = 0) -> CheckBox*;

        auto createDecorWidget(TrayLocation trayLoc, std::string_view name, std::string_view templateName) -> DecorWidget*;

        auto createProgressBar(TrayLocation trayLoc, std::string_view name, std::string_view caption,
            Ogre::Real width, Ogre::Real commentBoxWidth) -> ProgressBar*;

        /**
        Shows frame statistics widget set in the specified location.
        */
        void showFrameStats(TrayLocation trayLoc, size_t place = -1);

        /**
        Hides frame statistics widget set.
        */
        void hideFrameStats();

        auto areFrameStatsVisible() -> bool
        {
            return mFpsLabel != nullptr;
        }

        /**
        Toggles visibility of advanced statistics.
        */
        void toggleAdvancedFrameStats()
        {
            if (mFpsLabel) labelHit(mFpsLabel);
        }

        /**
        Shows logo in the specified location.
        */
        void showLogo(TrayLocation trayLoc, size_t place = -1);

        void hideLogo();

        auto isLogoVisible() noexcept -> bool
        {
            return mLogo != nullptr;
        }

        /**
        Pops up a message dialog with an OK button.
        */
        void showOkDialog(std::string_view caption, std::string_view message);

        /**
        Pops up a question dialog with Yes and No buttons.
        */
        void showYesNoDialog(std::string_view caption, std::string_view question);

        /**
        Hides whatever dialog is currently showing.
        */
        void closeDialog();

        /**
        Determines if any dialog is currently visible.
        */
        auto isDialogVisible() noexcept -> bool;

        /**
        Gets a widget from a tray by name.
        */
        auto getWidget(TrayLocation trayLoc, std::string_view name) -> Widget*;

        /**
        Gets a widget by name.
        */
        auto getWidget(std::string_view name) -> Widget*;

        /**
        Gets the number of widgets in total.
        */
        auto getNumWidgets() noexcept -> unsigned int;

        /**
        Gets all the widgets of a specific tray.
        */
        [[nodiscard]] auto getWidgets(TrayLocation trayLoc) const -> const WidgetList& {
            return mWidgets[std::to_underlying(trayLoc)];
        }

        /**
        Gets a widget's position in its tray.
        */
        auto locateWidgetInTray(Widget* widget) -> int;

        /**
        Destroys a widget.
        */
        void destroyWidget(Widget* widget);

        void destroyWidget(TrayLocation trayLoc, size_t place)
        {
            destroyWidget(mWidgets[std::to_underlying(trayLoc)][place]);
        }

        void destroyWidget(TrayLocation trayLoc, std::string_view name)
        {
            destroyWidget(getWidget(trayLoc, name));
        }

        void destroyWidget(std::string_view name)
        {
            destroyWidget(getWidget(name));
        }

        /**
        Destroys all widgets in a tray.
        */
        void destroyAllWidgetsInTray(TrayLocation trayLoc);

        /**
        Destroys all widgets.
        */
        void destroyAllWidgets();

        /**
        Adds a widget to a specified tray at given position, or at the end if unspecified or invalid
        */
        void moveWidgetToTray(Widget* widget, TrayLocation trayLoc, size_t place = -1);

        void moveWidgetToTray(std::string_view name, TrayLocation trayLoc, size_t place = -1)
        {
            moveWidgetToTray(getWidget(name), trayLoc, place);
        }

        void moveWidgetToTray(TrayLocation currentTrayLoc, std::string_view name, TrayLocation targetTrayLoc,
            size_t place = -1)
        {
            moveWidgetToTray(getWidget(currentTrayLoc, name), targetTrayLoc, place);
        }

        void moveWidgetToTray(TrayLocation currentTrayLoc, size_t currentPlace, TrayLocation targetTrayLoc,
            size_t targetPlace = -1)
        {
            moveWidgetToTray(mWidgets[std::to_underlying(currentTrayLoc)][currentPlace], targetTrayLoc, targetPlace);
        }

        /**
        Removes a widget from its tray. Same as moving it to the null tray.
        */
        void removeWidgetFromTray(Widget* widget)
        {
            moveWidgetToTray(widget, TrayLocation::NONE);
        }

        void removeWidgetFromTray(std::string_view name)
        {
            removeWidgetFromTray(getWidget(name));
        }

        void removeWidgetFromTray(TrayLocation trayLoc, std::string_view name)
        {
            removeWidgetFromTray(getWidget(trayLoc, name));
        }

        void removeWidgetFromTray(TrayLocation trayLoc, size_t place)
        {
            removeWidgetFromTray(mWidgets[std::to_underlying(trayLoc)][place]);
        }

        /**
        Removes all widgets from a widget tray.
        */
        void clearTray(TrayLocation trayLoc);

        /**
        Removes all widgets from all widget trays.
        */
        void clearAllTrays();

        /**
        Process frame events. Updates frame statistics widget set and deletes
        all widgets queued for destruction.
        */
        void frameRendered(const Ogre::FrameEvent& evt) override;

        void windowUpdate();

        void resourceGroupScriptingStarted(std::string_view groupName, size_t scriptCount) override
        {
            mLoadInc = mGroupInitProportion / float(scriptCount);
            windowUpdate();
        }

        void scriptParseStarted(std::string_view scriptName, bool& skipThisScript) override
        {
            windowUpdate();
        }

        void scriptParseEnded(std::string_view scriptName, bool skipped) override
        {
            windowUpdate();
        }

        void resourceGroupLoadStarted(std::string_view groupName, size_t resourceCount) override
        {
            mLoadInc = mGroupLoadProportion / float(resourceCount);
            windowUpdate();
        }

        void resourceLoadStarted(const Ogre::ResourcePtr& resource) override
        {
            windowUpdate();
        }

        void resourceLoadEnded() override
        {
            windowUpdate();
        }

        void customStageStarted(std::string_view description) override
        {
            windowUpdate();
        }

        void customStageEnded() override
        {
            windowUpdate();
        }

        /**
        Toggles visibility of advanced statistics.
        */
        void labelHit(Label* label) override;

        /**
        Destroys dialog widgets, notifies listener, and ends high priority session.
        */
        void buttonHit(Button* button) override;

        /**
        Processes mouse button down events. Returns true if the event was
        consumed and should not be passed on to other handlers.
        */
        auto mousePressed(const MouseButtonDownEvent& evt) noexcept -> bool override;

        /**
        Processes mouse button up events. Returns true if the event was
        consumed and should not be passed on to other handlers.
        */
        auto mouseReleased(const MouseButtonUpEvent& evt) noexcept -> bool override;

        /**
        Updates cursor position. Returns true if the event was
        consumed and should not be passed on to other handlers.
        */
        auto mouseMoved(const MouseMotionEvent& evt) noexcept -> bool override;

        auto mouseWheelRolled(const MouseWheelEvent& evt) noexcept -> bool override;

    protected:

        /**
        Internal method to prioritise / deprioritise expanded menus.
        */
        void setExpandedMenu(SelectMenu* m);

        Ogre::String mName;                   // name of this tray system
        Ogre::RenderWindow* mWindow;          // render window
        Ogre::Overlay* mBackdropLayer;        // backdrop layer
        Ogre::Overlay* mTraysLayer;           // widget layer
        Ogre::Overlay* mPriorityLayer;        // top priority layer
        Ogre::Overlay* mCursorLayer;          // cursor layer
        Ogre::OverlayContainer* mBackdrop;    // backdrop
        Ogre::OverlayContainer* mTrays[10];   // widget trays
        WidgetList mWidgets[10];              // widgets
        std::vector<::std::unique_ptr<Widget>> mWidgetDeathRow;           // widget queue for deletion
        Ogre::OverlayContainer* mCursor;      // cursor
        TrayListener* mListener;           // tray listener
        Ogre::Real mWidgetPadding{8};            // widget padding
        Ogre::Real mWidgetSpacing{2};            // widget spacing
        Ogre::Real mTrayPadding{0};              // tray padding
        bool mTrayDrag{false};                       // a mouse press was initiated on a tray
        SelectMenu* mExpandedMenu{nullptr};            // top priority expanded menu widget
        TextBox* mDialog{nullptr};                     // top priority dialog widget
        Ogre::OverlayContainer* mDialogShade; // top priority dialog shade
        Button* mOk{nullptr};                          // top priority OK button
        Button* mYes{nullptr};                         // top priority Yes button
        Button* mNo{nullptr};                          // top priority No button
        bool mCursorWasVisible{false};               // cursor state before showing dialog
        Label* mFpsLabel{nullptr};                     // FPS label
        ParamsPanel* mStatsPanel{nullptr};             // frame stats panel
        DecorWidget* mLogo{nullptr};                   // logo
        Ogre::Real mGroupInitProportion{0.0f};      // proportion of load job assigned to initialising one resource group
        Ogre::Real mGroupLoadProportion{0.0f};      // proportion of load job assigned to loading one resource group
        Ogre::Real mLoadInc{0.0f};                  // loading increment
        Ogre::GuiHorizontalAlignment mTrayWidgetAlign[10];   // tray widget alignments
        Ogre::Timer* mTimer;                  // Root::getSingleton().getTimer()
        unsigned long mLastStatUpdateTime;    // The last time the stat text were updated
        Ogre::Vector2 mCursorPos;             // current cursor position

    };
    /** @} */
    /** @} */
    /** @} */
}
