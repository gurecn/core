/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_SDEXT_SOURCE_PRESENTER_PRESENTERNOTESVIEW_HXX
#define INCLUDED_SDEXT_SOURCE_PRESENTER_PRESENTERNOTESVIEW_HXX

#include "PresenterController.hxx"
#include "PresenterToolBar.hxx"
#include "PresenterViewFactory.hxx"
#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/compbase.hxx>
#include <com/sun/star/awt/XWindowListener.hpp>
#include <com/sun/star/drawing/XDrawPage.hpp>
#include <com/sun/star/drawing/XDrawView.hpp>
#include <framework/AbstractView.hxx>
#include <ResourceId.hxx>
#include <com/sun/star/frame/XController.hpp>
#include <rtl/ref.hxx>
#include <memory>

namespace sd { class DrawController; }

namespace sdext::presenter {

class PresenterButton;
class PresenterScrollBar;
class PresenterTextView;

typedef cppu::ImplInheritanceHelper<
    sd::framework::AbstractView,
    css::awt::XWindowListener,
    css::awt::XPaintListener,
    css::drawing::XDrawView,
    css::awt::XKeyListener
    > PresenterNotesViewInterfaceBase;

/** A drawing framework view of the notes of a slide.  At the moment this is
    a simple text view that does not show the original formatting of the
    notes text.
*/
class PresenterNotesView
    : public PresenterNotesViewInterfaceBase,
      public CachablePresenterView
{
public:
    explicit PresenterNotesView (
        const css::uno::Reference<css::uno::XComponentContext>& rxContext,
        const rtl::Reference<sd::framework::ResourceId>& rxViewId,
        const ::rtl::Reference<::sd::DrawController>& rxController,
        const ::rtl::Reference<PresenterController>& rpPresenterController);
    virtual ~PresenterNotesView() override;

    virtual void disposing(std::unique_lock<std::mutex>&) override;

    /** Typically called from setCurrentSlide() with the notes page that is
        associated with the slide given to setCurrentSlide().

        Iterates over all text shapes on the given notes page and displays
        the concatenated text of these.
    */
    void SetSlide (
        const css::uno::Reference<css::drawing::XDrawPage>& rxNotesPage);

    void ChangeFontSize (const sal_Int32 nSizeChange);

    const std::shared_ptr<PresenterTextView>& GetTextView() const;

    // lang::XEventListener

    using WeakComponentImplHelperBase::disposing;
    virtual void SAL_CALL
        disposing (const css::lang::EventObject& rEventObject) override;

    // XWindowListener

    virtual void SAL_CALL windowResized (const css::awt::WindowEvent& rEvent) override;

    virtual void SAL_CALL windowMoved (const css::awt::WindowEvent& rEvent) override;

    virtual void SAL_CALL windowShown (const css::lang::EventObject& rEvent) override;

    virtual void SAL_CALL windowHidden (const css::lang::EventObject& rEvent) override;

    // XPaintListener

    virtual void SAL_CALL windowPaint (const css::awt::PaintEvent& rEvent) override;

    // AbstractResource

    virtual rtl::Reference<sd::framework::ResourceId> getResourceId() override;

    virtual bool isAnchorOnly() override;

    // XDrawView

    virtual void SAL_CALL setCurrentPage (
        const css::uno::Reference<css::drawing::XDrawPage>& rxSlide) override;

    virtual css::uno::Reference<css::drawing::XDrawPage> SAL_CALL getCurrentPage() override;

    // XKeyListener

    virtual void SAL_CALL keyPressed (const css::awt::KeyEvent& rEvent) override;
    virtual void SAL_CALL keyReleased (const css::awt::KeyEvent& rEvent) override;

private:
    rtl::Reference<sd::framework::ResourceId> mxViewId;
    ::rtl::Reference<PresenterController> mpPresenterController;
    css::uno::Reference<css::awt::XWindow> mxParentWindow;
    css::uno::Reference<css::rendering::XCanvas> mxCanvas;
    css::uno::Reference<css::drawing::XDrawPage> mxCurrentNotesPage;
    ::rtl::Reference<PresenterScrollBar> mpScrollBar;
    css::uno::Reference<css::awt::XWindow> mxToolBarWindow;
    css::uno::Reference<css::rendering::XCanvas> mxToolBarCanvas;
    ::rtl::Reference<PresenterToolBar> mpToolBar;
    ::rtl::Reference<PresenterButton> mpCloseButton;
    css::util::Color maSeparatorColor;
    sal_Int32 mnSeparatorYLocation;
    css::geometry::RealRectangle2D maTextBoundingBox;
    SharedBitmapDescriptor mpBackground;
    double mnTop;
    PresenterTheme::SharedFontDescriptor mpFont;
    std::shared_ptr<PresenterTextView> mpTextView;

    void CreateToolBar (
        const css::uno::Reference<css::uno::XComponentContext>& rxContext,
        const ::rtl::Reference<PresenterController>& rpPresenterController);
    void Layout();
    void Paint (const css::awt::Rectangle& rUpdateBox);
    void PaintToolBar (const css::awt::Rectangle& rUpdateBox);
    void PaintText (const css::awt::Rectangle& rUpdateBox);
    void Invalidate();
    void Scroll (const double nDistance);
    void SetTop (const double nTop);
    void UpdateScrollBar();
};

} // end of namespace ::sdext::presenter

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
