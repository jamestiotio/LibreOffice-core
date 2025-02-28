/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <swmodeltestbase.hxx>

#include <com/sun/star/text/VertOrientation.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>

#include <svx/svdpage.hxx>

#include <wrtsh.hxx>
#include <fmtanchr.hxx>
#include <IDocumentDrawModelAccess.hxx>
#include <drawdoc.hxx>
#include <dcontact.hxx>
#include <frameformats.hxx>
#include <unotxdoc.hxx>
#include <docsh.hxx>
#include <swdtflvr.hxx>
#include <caption.hxx>
#include <view.hxx>
#include <formatflysplit.hxx>

/// Covers sw/source/core/frmedt/ fixes.
class SwCoreFrmedtTest : public SwModelTestBase
{
public:
    SwCoreFrmedtTest()
        : SwModelTestBase("/sw/qa/core/frmedt/data/")
    {
    }
};

CPPUNIT_TEST_FIXTURE(SwCoreFrmedtTest, testTextboxReanchor)
{
    // Load a document with a textframe and a textbox(shape+textframe).
    createSwDoc("textbox-reanchor.odt");
    SwXTextDocument* pTextDoc = dynamic_cast<SwXTextDocument*>(mxComponent.get());
    SwDoc* pDoc = pTextDoc->GetDocShell()->GetDoc();
    SdrPage* pDrawPage = pDoc->getIDocumentDrawModelAccess().GetDrawModel()->GetPage(0);
    SdrObject* pDrawShape = pDrawPage->GetObj(1);
    CPPUNIT_ASSERT_EQUAL(OUString("draw shape"), pDrawShape->GetName());

    // Select the shape of the textbox.
    Point aPoint;
    SwWrtShell* pShell = pDoc->GetDocShell()->GetWrtShell();
    pShell->SelectObj(aPoint, /*nFlag=*/0, pDrawShape);

    // Anchor the shape of the textbox into its own textframe.
    SdrObject* pTextFrameObj = pDrawPage->GetObj(2);
    SwFrameFormat* pTextFrameFormat = FindFrameFormat(pTextFrameObj);
    CPPUNIT_ASSERT_EQUAL(OUString("Frame2"), pTextFrameFormat->GetName());
    SwFrameFormat* pDrawShapeFormat = FindFrameFormat(pDrawShape);
    SwNodeOffset nOldAnchor = pDrawShapeFormat->GetAnchor().GetAnchorNode()->GetIndex();
    pShell->FindAnchorPos(pTextFrameObj->GetLastBoundRect().Center(), true);
    SwNodeOffset nNewAnchor = pDrawShapeFormat->GetAnchor().GetAnchorNode()->GetIndex();
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 6
    // - Actual  : 9
    // i.e. SwFEShell allowed to anchor the textframe of a textbox into itself.
    CPPUNIT_ASSERT_EQUAL(nOldAnchor, nNewAnchor);
}

CPPUNIT_TEST_FIXTURE(SwCoreFrmedtTest, testVertPosFromBottomBoundingBox)
{
    // Insert a shape and anchor it vertically in a way, so its position is from the top of the page
    // bottom margin area.
    createSwDoc();
    uno::Reference<css::lang::XMultiServiceFactory> xFactory(mxComponent, uno::UNO_QUERY);
    uno::Reference<drawing::XShape> xShape(
        xFactory->createInstance("com.sun.star.drawing.RectangleShape"), uno::UNO_QUERY);
    xShape->setSize(awt::Size(10000, 10000));
    uno::Reference<beans::XPropertySet> xShapeProps(xShape, uno::UNO_QUERY);
    xShapeProps->setPropertyValue("AnchorType", uno::Any(text::TextContentAnchorType_AT_CHARACTER));
    xShapeProps->setPropertyValue("VertOrient", uno::Any(text::VertOrientation::NONE));
    xShapeProps->setPropertyValue("VertOrientRelation",
                                  uno::Any(text::RelOrientation::PAGE_PRINT_AREA_BOTTOM));
    xShapeProps->setPropertyValue("VertOrientPosition", uno::Any(static_cast<sal_Int32>(-11000)));
    uno::Reference<drawing::XDrawPageSupplier> xDrawPageSupplier(mxComponent, uno::UNO_QUERY);
    xDrawPageSupplier->getDrawPage()->add(xShape);

    // Get the absolute position of the top of the page bottom margin area.
    xmlDocUniquePtr pXmlDoc = parseLayoutDump();
    SwTwips nPagePrintAreaBottom = getXPath(pXmlDoc, "//page/infos/prtBounds", "bottom").toInt32();

    // Calculate the allowed bounding box of the shape, e.g. the shape's position & size dialog uses
    // this to limit the vertical position to sensible values.
    SwXTextDocument* pTextDoc = dynamic_cast<SwXTextDocument*>(mxComponent.get());
    SwWrtShell* pWrtShell = pTextDoc->GetDocShell()->GetWrtShell();
    SwRect aBoundRect;
    RndStdIds eAnchorType = RndStdIds::FLY_AT_CHAR;
    SwDoc* pDoc = pTextDoc->GetDocShell()->GetDoc();
    const auto& rFrameFormats = *pDoc->GetFrameFormats();
    const SwFormatAnchor* pFormatAhchor = &rFrameFormats[0]->GetAnchor();
    sal_Int16 eHoriRelOrient = text::RelOrientation::PAGE_FRAME;
    sal_Int16 eVertRelOrient = text::RelOrientation::PAGE_PRINT_AREA_BOTTOM;
    bool bFollowTextFlow = false;
    bool bMirror = false;
    Size aPercentSize;
    pWrtShell->CalcBoundRect(aBoundRect, eAnchorType, eHoriRelOrient, eVertRelOrient, pFormatAhchor,
                             bFollowTextFlow, bMirror, nullptr, &aPercentSize);

    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: -14705
    // - Actual  : -1134
    // i.e. UI did not allow anchoring a shape 10cm above the bottom of the page due to wrong
    // bounding box.
    CPPUNIT_ASSERT_EQUAL(-1 * nPagePrintAreaBottom, aBoundRect.Pos().getY());
}

CPPUNIT_TEST_FIXTURE(SwCoreFrmedtTest, testPasteFlyInTextBox)
{
    // Given a document that contains a textbox, which contains an sw image (fly frame)
    createSwDoc("paste-fly-in-textbox.docx");
    SwXTextDocument* pTextDoc = dynamic_cast<SwXTextDocument*>(mxComponent.get());
    SwDocShell* pDocShell = pTextDoc->GetDocShell();
    SwWrtShell* pWrtShell = pDocShell->GetWrtShell();
    SwDoc* pDoc = pDocShell->GetDoc();
    SdrPage* pPage = pDoc->getIDocumentDrawModelAccess().GetDrawModel()->GetPage(0);
    SdrObject* pObject = pPage->GetObj(0);
    pWrtShell->SelectObj(Point(), 0, pObject);
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), pDoc->GetSpzFrameFormats()->GetFormatCount());
    rtl::Reference<SwTransferable> pTransfer = new SwTransferable(*pWrtShell);
    pTransfer->Cut();
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(0), pDoc->GetSpzFrameFormats()->GetFormatCount());
    TransferableDataHelper aHelper(pTransfer);

    // When pasting that to an empty document.
    SwTransferable::Paste(*pWrtShell, aHelper);

    // Then we should have the image only once: 3 formats (draw+fly formats for the textbox and a
    // fly format for the image).
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 3
    // - Actual  : 4
    // i.e. the image was pasted twice.
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(3), pDoc->GetSpzFrameFormats()->GetFormatCount());
}

CPPUNIT_TEST_FIXTURE(SwCoreFrmedtTest, testTextBoxSelectCursorPos)
{
    // Given a document with a fly+draw format pair (textbox):
    createSwDoc("paste-fly-in-textbox.docx");

    // When selecting the fly format:
    SwDoc* pDoc = getSwDoc();
    SdrPage* pPage = pDoc->getIDocumentDrawModelAccess().GetDrawModel()->GetPage(0);
    SdrObject* pFlyObject = pPage->GetObj(1);
    SwContact* pFlyContact = static_cast<SwContact*>(pFlyObject->GetUserCall());
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt16>(RES_FLYFRMFMT), pFlyContact->GetFormat()->Which());
    SwWrtShell* pWrtShell = pDoc->GetDocShell()->GetWrtShell();
    pWrtShell->SelectObj(Point(), 0, pFlyObject);

    // Then make sure the cursor is the anchor of the draw format:
    SdrObject* pDrawObject = pPage->GetObj(0);
    SwDrawContact* pDrawContact = static_cast<SwDrawContact*>(pDrawObject->GetUserCall());
    SwFrameFormat* pDrawFormat = pDrawContact->GetFormat();
    CPPUNIT_ASSERT_EQUAL(static_cast<sal_uInt16>(RES_DRAWFRMFMT), pDrawFormat->Which());
    SwNodeOffset nAnchor = pDrawFormat->GetAnchor().GetContentAnchor()->GetNode().GetIndex();
    SwNodeOffset nCursor = pWrtShell->GetCurrentShellCursor().GetPointNode().GetIndex();
    // Without the accompanying fix in place, this test would have failed with:
    // - Expected: 15 (anchor of draw format)
    // - Actual  : 6 (in-fly-format position)
    // i.e. the cursor had a broken position after trying to select the fly format.
    CPPUNIT_ASSERT_EQUAL(nAnchor, nCursor);
}

CPPUNIT_TEST_FIXTURE(SwCoreFrmedtTest, testSplitFlyInsertCaption)
{
    // Given a document with a full-page floating table:
    createSwDoc("floating-table-caption.docx");

    // When trying to insert a caption below that table:
    SwWrtShell* pWrtShell = getSwDocShell()->GetWrtShell();
    pWrtShell->GotoTable("Table1");
    InsCaptionOpt aOpt;
    SwView& rView = pWrtShell->GetView();
    aOpt.SetCategory("Table");
    aOpt.SetCaption("Numbers English-German");
    // After, not before.
    aOpt.SetPos(1);
    // Without the accompanying fix in place, this call never finished, layout didn't handle content
    // after the table in a floating table.
    rView.InsertCaption(&aOpt);

    // Then make sure the insertion finishes and now this is just a plain table-in-frame:
    SwDoc* pDoc = getSwDoc();
    sw::SpzFrameFormats& rFlys = *pDoc->GetSpzFrameFormats();
    sw::SpzFrameFormat* pFly = rFlys[0];
    CPPUNIT_ASSERT(!pFly->GetAttrSet().GetFlySplit().GetValue());
}

CPPUNIT_PLUGIN_IMPLEMENT();

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
