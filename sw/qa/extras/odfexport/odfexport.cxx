/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <algorithm>
#include <memory>
#include <swmodeltestbase.hxx>

#include <com/sun/star/awt/FontSlant.hpp>
#include <com/sun/star/awt/Gradient2.hpp>
#include <com/sun/star/container/XIndexReplace.hpp>
#include <com/sun/star/drawing/FillStyle.hpp>
#include <com/sun/star/drawing/PointSequenceSequence.hpp>
#include <com/sun/star/drawing/GraphicExportFilter.hpp>
#include <com/sun/star/drawing/XGraphicExportFilter.hpp>
#include <com/sun/star/drawing/BarCode.hpp>
#include <com/sun/star/drawing/BarCodeErrorCorrection.hpp>
#include <com/sun/star/table/ShadowFormat.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/text/RelOrientation.hpp>
#include <com/sun/star/text/XDocumentIndex.hpp>
#include <com/sun/star/drawing/TextVerticalAdjust.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <officecfg/Office/Common.hxx>
#include <com/sun/star/document/XEmbeddedObjectSupplier.hpp>
#include <com/sun/star/text/XTextEmbeddedObjectsSupplier.hpp>
#include <com/sun/star/text/XTextField.hpp>
#include <com/sun/star/text/WritingMode2.hpp>
#include <com/sun/star/util/XModifiable.hpp>
#include <com/sun/star/text/XTextFieldsSupplier.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/document/XStorageBasedDocument.hpp>
#include <com/sun/star/text/XTextFramesSupplier.hpp>
#include <com/sun/star/text/XDocumentIndexesSupplier.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/util/XRefreshable.hpp>
#include <com/sun/star/container/XContentEnumerationAccess.hpp>
#include <com/sun/star/text/XTextTable.hpp>

#include <comphelper/storagehelper.hxx>
#include <comphelper/fileformat.h>
#include <comphelper/propertysequence.hxx>
#include <comphelper/documentconstants.hxx>
#include <unotools/streamwrap.hxx>
#include <svl/PasswordHelper.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <vcl/filter/PDFiumLibrary.hxx>
#include <comphelper/scopeguard.hxx>
#include <basegfx/utils/gradienttools.hxx>
#include <docmodel/uno/UnoGradientTools.hxx>

#include <docufld.hxx> // for SwHiddenTextField::ParseIfFieldDefinition() method call
#include <unoprnms.hxx>
#include <sortedobjs.hxx>
#include <flyfrm.hxx>
#include <ftnidx.hxx>
#include <txtftn.hxx>
#include <unotxdoc.hxx>
#include <docsh.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <rootfrm.hxx>
#include <o3tl/string_view.hxx>

class Test : public SwModelTestBase
{
public:
    Test() : SwModelTestBase("/sw/qa/extras/odfexport/data/", "writer8") {}
};

CPPUNIT_TEST_FIXTURE(Test, testMathObjectFlatExport)
{
    comphelper::ScopeGuard g([this]() {
        mpFilter = "writer8";
        std::shared_ptr<comphelper::ConfigurationChanges> pBatch(
            comphelper::ConfigurationChanges::create());
        officecfg::Office::Common::Cache::Writer::OLE_Objects::set(20, pBatch);
        return pBatch->commit();
    });
    mpFilter = "OpenDocument Text Flat XML"; // doesn't happen with ODF package
    std::shared_ptr<comphelper::ConfigurationChanges> pBatch(
        comphelper::ConfigurationChanges::create());
    officecfg::Office::Common::Cache::Writer::OLE_Objects::set(1, pBatch);
    pBatch->commit();
    loadAndReload("2_MathType3.docx");

    uno::Reference<util::XModifiable> xModifiable(mxComponent, uno::UNO_QUERY);
    CPPUNIT_ASSERT(!xModifiable->isModified());
    // see above, set the OLE cache to 1 for this test
    // and the problem was that the formulas that were in the cache
    // (the second one) were lost
    OUString formula1(getFormula(getRun(getParagraph(1), 1)));
    CPPUNIT_ASSERT_EQUAL(OUString(" size 12{1+1=2} {}"), formula1);
    OUString formula2(getFormula(getRun(getParagraph(2), 1)));
    CPPUNIT_ASSERT_EQUAL(OUString(" size 12{2+2=4} {}"), formula2);
}

DECLARE_ODFEXPORT_TEST(testTdf144319, "tdf144319.odt")
{
    CPPUNIT_ASSERT_EQUAL(7, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    OUString formula1(getFormula(getRun(getParagraph(3), 1)));
    CPPUNIT_ASSERT_EQUAL(OUString("{ x = frac { { - b +- sqrt { b ^ 2 - 4 a c } } } { { 2 a } } }"), formula1);
    OUString formula2(getFormula(getRun(getParagraph(4), 1)));
    CPPUNIT_ASSERT_EQUAL(OUString("{ sum csup n csub { i = 1 } i ^ 3 = left ( frac { { n left ( { n + 1 } right ) } } { 2 } right ) ^ 2 }"), formula2);
    OUString formula3(getFormula(getRun(getParagraph(5), 1)));
    CPPUNIT_ASSERT_EQUAL(OUString("{ sum ^ n _ { i = 1 } i ^ 3 = left ( frac { { n left ( { n + 1 } right ) } } { 2 } right ) ^ 2 }"), formula3);
    OUString formula4(getFormula(getRun(getParagraph(6), 1)));
    CPPUNIT_ASSERT_EQUAL(OUString("{ sum ^ n _ { i = 1 } i ^ 3 = left ( frac { { n left ( { n + 1 } right ) } } { 2 } right ) ^ 2 }"), formula4);

    // Without the fix in place, this test would have failed with
    // - the property is of unexpected type or void: Model
    OUString formula5(getFormula(getRun(getParagraph(7), 1)));
    CPPUNIT_ASSERT_EQUAL(OUString("{ y ^ 2 { nitalic m p } = left ( { x ^ 3 + 7 } right ) { nitalic m p } }"), formula5);
}

static void testTdf43569_CheckIfFieldParse()
{
    {
        OUString paramCondition;
        OUString paramTrue;
        OUString paramFalse;

        SwHiddenTextField::ParseIfFieldDefinition(u"IF A B C", paramCondition, paramTrue, paramFalse);

        CPPUNIT_ASSERT_EQUAL(OUString("A"), paramCondition);
        CPPUNIT_ASSERT_EQUAL(OUString("B"), paramTrue);
        CPPUNIT_ASSERT_EQUAL(OUString("C"), paramFalse);
    }

    {
        OUString paramCondition;
        OUString paramTrue;
        OUString paramFalse;

        SwHiddenTextField::ParseIfFieldDefinition(u"  IF AAA BBB CCC  ", paramCondition, paramTrue, paramFalse);

        CPPUNIT_ASSERT_EQUAL(OUString("AAA"), paramCondition);
        CPPUNIT_ASSERT_EQUAL(OUString("BBB"), paramTrue);
        CPPUNIT_ASSERT_EQUAL(OUString("CCC"), paramFalse);
    }

    {
        OUString paramCondition;
        OUString paramTrue;
        OUString paramFalse;

        SwHiddenTextField::ParseIfFieldDefinition(u"  IF AAA \"BBB\" \"CCC\"  ", paramCondition, paramTrue, paramFalse);

        CPPUNIT_ASSERT_EQUAL(OUString("AAA"), paramCondition);
        CPPUNIT_ASSERT_EQUAL(OUString("BBB"), paramTrue);
        CPPUNIT_ASSERT_EQUAL(OUString("CCC"), paramFalse);
    }

    // true-case and false-case have spaces inside
    {
        OUString paramCondition;
        OUString paramTrue;
        OUString paramFalse;

        SwHiddenTextField::ParseIfFieldDefinition(u"  IF A A A \"B B B\" \"C C C\"  ", paramCondition, paramTrue, paramFalse);

        CPPUNIT_ASSERT_EQUAL(OUString("A A A"), paramCondition);
        CPPUNIT_ASSERT_EQUAL(OUString("B B B"), paramTrue);
        CPPUNIT_ASSERT_EQUAL(OUString("C C C"), paramFalse);
    }

    // true-case and false-case have leading/trailing space
    {
        OUString paramCondition;
        OUString paramTrue;
        OUString paramFalse;

        SwHiddenTextField::ParseIfFieldDefinition(u"IF A1 A2 A3 \"B1 B2 \" \" C1 C2\"  ", paramCondition, paramTrue, paramFalse);

        CPPUNIT_ASSERT_EQUAL(OUString("A1 A2 A3"), paramCondition);
        CPPUNIT_ASSERT_EQUAL(OUString("B1 B2 "), paramTrue);
        CPPUNIT_ASSERT_EQUAL(OUString(" C1 C2"), paramFalse);
    }

    // true-case and false-case are empty
    {
        OUString paramCondition;
        OUString paramTrue;
        OUString paramFalse;

        SwHiddenTextField::ParseIfFieldDefinition(u"IF condition \"\" \"\"  ", paramCondition, paramTrue, paramFalse);

        CPPUNIT_ASSERT_EQUAL(OUString("condition"), paramCondition);
        CPPUNIT_ASSERT_EQUAL(OUString(""), paramTrue);
        CPPUNIT_ASSERT_EQUAL(OUString(""), paramFalse);
    }
}

// Input document contains only one IF-field,
// and it should be imported as com.sun.star.text.TextField.ConditionalText in any case,
// instead of insertion of the pair of two field-marks: <field:fieldmark-start> + <field:fieldmark-end>.
CPPUNIT_TEST_FIXTURE(Test, testTdf43569)
{
    loadAndReload("tdf43569_conditionalfield.doc");
    // check if our parser is valid
    testTdf43569_CheckIfFieldParse();

    // now check field creation during import
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // at least one field should be detected
    CPPUNIT_ASSERT(xFields->hasMoreElements());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf130314)
{
    loadAndReload("tdf130314.docx");
    // Without the fix in place, this test would have hung
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf133487)
{
    loadAndReload("MadeByLO7.odt");
    CPPUNIT_ASSERT_EQUAL(3, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("content.xml");
    // shape in background has lowest index
    assertXPath(pXmlDoc, "/office:document-content/office:body/office:text/text:p[2]/draw:custom-shape", "z-index", "0");
    assertXPath(pXmlDoc, "/office:document-content/office:automatic-styles/style:style[@style:name = /office:document-content/office:body/office:text/text:p[2]/draw:custom-shape[@draw:z-index = '0']/attribute::draw:style-name]/style:graphic-properties", "run-through", "background");
    // shape in foreground, previously index 1
    assertXPath(pXmlDoc, "/office:document-content/office:body/office:text/text:p[1]/draw:custom-shape", "z-index", "2");
    assertXPath(pXmlDoc, "/office:document-content/office:automatic-styles/style:style[@style:name = /office:document-content/office:body/office:text/text:p[1]/draw:custom-shape[@draw:z-index = '2']/attribute::draw:style-name]/style:graphic-properties", "run-through", "foreground");
    // shape in foreground, previously index 0
    assertXPath(pXmlDoc, "/office:document-content/office:body/office:text/text:p[3]/draw:custom-shape", "z-index", "1");
    assertXPath(pXmlDoc, "/office:document-content/office:automatic-styles/style:style[@style:name = /office:document-content/office:body/office:text/text:p[3]/draw:custom-shape[@draw:z-index = '1']/attribute::draw:style-name]/style:graphic-properties", "run-through", "foreground");
}

CPPUNIT_TEST_FIXTURE(Test, testTdf141467)
{
    loadAndReload("Formcontrol needs high z-index.odt");
    CPPUNIT_ASSERT_EQUAL(2, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("content.xml");
    // shape in foreground has lowest index
    assertXPath(pXmlDoc, "/office:document-content/office:body/office:text/text:p[2]/draw:custom-shape", "z-index", "0");
    assertXPath(pXmlDoc, "/office:document-content/office:automatic-styles/style:style[@style:name = /office:document-content/office:body/office:text/text:p[2]/draw:custom-shape[@draw:z-index = '0']/attribute::draw:style-name]/style:graphic-properties", "run-through", "foreground");
    // form control, previously index 0
    assertXPath(pXmlDoc, "/office:document-content/office:body/office:text/text:p[2]/draw:control", "z-index", "1");
    // no run-through on form's style
    assertXPath(pXmlDoc, "/office:document-content/office:automatic-styles/style:style[@style:name = /office:document-content/office:body/office:text/text:p[2]/draw:control[@draw:z-index = '1']/attribute::draw:style-name]/style:graphic-properties/attribute::run-through", 0);
}

DECLARE_ODFEXPORT_TEST(testTdf139126, "tdf139126.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<text::XTextTablesSupplier> xSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xTables = xSupplier->getTextTables();
    uno::Reference<text::XTextTable> xTable(xTables->getByName("Table1"), uno::UNO_QUERY);

    uno::Reference<text::XTextRange> xD2(xTable->getCellByName("D2"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("4.0"), xD2->getString());

    // Without the fix in place, this test would have failed with
    // - Expected: ** Expression is faulty **
    // - Actual  : 17976931348623200...
    uno::Reference<text::XTextRange> xE2(xTable->getCellByName("E2"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("** Expression is faulty **"), xE2->getString());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf125877)
{
    loadAndReload("tdf95806.docx");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<text::XTextTablesSupplier> xSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xTables(xSupplier->getTextTables(), uno::UNO_QUERY);

    // This was 0 (lost table during ODT export in footnotes)
    // Note: fix also tdf#95806: painting table layout is correct
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), xTables->getCount());

    // floating table: there is a frame now
    uno::Reference<text::XTextFramesSupplier> xTextFramesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xIndexAccess(xTextFramesSupplier->getTextFrames(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), xIndexAccess->getCount());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf150149)
{
    loadAndReload("tdf150149.fodt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("content.xml");
    // This was 0 (lost table header in multi-column section)
    assertXPath(pXmlDoc, "//table:table-header-rows", 1);
    assertXPath(pXmlDoc, "//table:table-header-rows/table:table-row/table:table-cell", 3);
}

DECLARE_ODFEXPORT_TEST(testTdf103567, "tdf103567.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<drawing::XShape> const xShape(getShape(1));

    // contour wrap polygon
    css::drawing::PointSequenceSequence const pointss(
        getProperty<css::drawing::PointSequenceSequence>(xShape, "ContourPolyPolygon"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), pointss.getLength());
    // for some reason this property exists with 199 points if it wasn't
    // imported, that would be a fail
    CPPUNIT_ASSERT_EQUAL(sal_Int32(5), pointss[0].getLength());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(   0), pointss[0][0].X);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2672), pointss[0][0].Y);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(   0), pointss[0][1].X);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1111), pointss[0][1].Y);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2672), pointss[0][2].X);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1111), pointss[0][2].Y);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2672), pointss[0][3].X);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2672), pointss[0][3].Y);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(   0), pointss[0][4].X);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2672), pointss[0][4].Y);

    // image map, one rectangle
    uno::Reference<container::XIndexContainer> const xImageMap(
        getProperty<uno::Reference<container::XIndexContainer>>(xShape, "ImageMap"));

    uno::Reference<beans::XPropertySet> const xEntry(xImageMap->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/"), getProperty<OUString>(xEntry, "URL"));
    awt::Rectangle const rect(getProperty<awt::Rectangle>(xEntry, "Boundary"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32( 726), rect.X);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1718), rect.Y);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1347), rect.Width);
    CPPUNIT_ASSERT_EQUAL(sal_Int32( 408), rect.Height);
}

CPPUNIT_TEST_FIXTURE(Test, testUserFieldDecl)
{
    loadAndReload("user-field-decl.odt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");
    // Without the accompanying fix in place, this test would have failed with 'Expected: 2;
    // Actual: 1', i.e. the in-table field had no declaration (in the header), while the
    // outside-table one had the declaration.
    assertXPath(pXmlDoc, "//style:header/text:user-field-decls/text:user-field-decl", 2);
}

CPPUNIT_TEST_FIXTURE(Test, testUserFieldDeclFly)
{
    loadAndReload("user-field-decl-fly.odt");
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");
    // Without the accompanying fix in place, this test would have failed with 'Expected: 2;
    // Actual: 1', i.e. the in-textframe field had no declaration (in the header), while the
    // outside-textframe one had the declaration.
    assertXPath(pXmlDoc, "//style:header/text:user-field-decls/text:user-field-decl", 2);
}

DECLARE_ODFEXPORT_TEST(testFramebackgrounds, "framebackgrounds.odt")
{
    CPPUNIT_ASSERT_EQUAL(16, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
   //Counting the Number of Frames and checking with the expected count
    uno::Reference<text::XTextFramesSupplier> xTextFramesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xIndexAccess(xTextFramesSupplier->getTextFrames(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(16), xIndexAccess->getCount());
    uno::Reference<drawing::XShape> xTextFrame;
    awt::Gradient aGradientxTextFrame;
    //Frame 1
    xTextFrame = getShape(1);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_NONE, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    //Frame 2
    xTextFrame = getShape(2);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_SOLID, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(Color(0x006600), getProperty<Color>(xTextFrame, "FillColor"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    //Frame 3
    xTextFrame = getShape(3);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_SOLID, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(Color(0x006600), getProperty<Color>(xTextFrame, "FillColor"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(45), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    //Frame 4
    xTextFrame = getShape(4);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_SOLID, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(Color(0x579D1C), getProperty<Color>(xTextFrame, "FillColor"));
    aGradientxTextFrame = getProperty<awt::Gradient>(xTextFrame, "FillTransparenceGradient");
    CPPUNIT_ASSERT_EQUAL(css::awt::GradientStyle_LINEAR, aGradientxTextFrame.Style);
    //Frame 5
    xTextFrame = getShape(5);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_GRADIENT, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    CPPUNIT_ASSERT_EQUAL(OUString("Subtle Tango Green"), getProperty<OUString>(xTextFrame, "FillGradientName"));
    //Frame 6
    xTextFrame = getShape(6);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_GRADIENT, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Subtle Tango Green"), getProperty<OUString>(xTextFrame, "FillGradientName"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(45), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    //Frame 7
    xTextFrame = getShape(7);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_GRADIENT, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Subtle Tango Green"), getProperty<OUString>(xTextFrame, "FillGradientName"));
    aGradientxTextFrame = getProperty<awt::Gradient>(xTextFrame, "FillTransparenceGradient");
    CPPUNIT_ASSERT_EQUAL(css::awt::GradientStyle_LINEAR, aGradientxTextFrame.Style);
    //Frame 8
    xTextFrame = getShape(8);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_HATCH, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Black 0 Degrees"), getProperty<OUString>(xTextFrame, "FillHatchName"));
    CPPUNIT_ASSERT_EQUAL(false, getProperty<bool>(xTextFrame, "FillBackground"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    //Frame 9
    xTextFrame = getShape(9);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_HATCH, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Black 0 Degrees"), getProperty<OUString>(xTextFrame, "FillHatchName"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xTextFrame, "FillBackground"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    //Frame 10
    xTextFrame = getShape(10);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_HATCH, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Black 0 Degrees"), getProperty<OUString>(xTextFrame, "FillHatchName"));
    CPPUNIT_ASSERT_EQUAL(false, getProperty<bool>(xTextFrame, "FillBackground"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(45), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    //Frame 11
    xTextFrame = getShape(11);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_HATCH, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Black 0 Degrees"), getProperty<OUString>(xTextFrame, "FillHatchName"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xTextFrame, "FillBackground"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(45), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    //Frame 12
    xTextFrame = getShape(12);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_HATCH, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Black 0 Degrees"), getProperty<OUString>(xTextFrame, "FillHatchName"));
    CPPUNIT_ASSERT_EQUAL(false, getProperty<bool>(xTextFrame, "FillBackground"));
    aGradientxTextFrame = getProperty<awt::Gradient>(xTextFrame, "FillTransparenceGradient");
    CPPUNIT_ASSERT_EQUAL(css::awt::GradientStyle_LINEAR, aGradientxTextFrame.Style);
    //Frame 13
    xTextFrame = getShape(13);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_HATCH, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Black 0 Degrees"), getProperty<OUString>(xTextFrame, "FillHatchName"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xTextFrame, "FillBackground"));
    aGradientxTextFrame = getProperty<awt::Gradient>(xTextFrame, "FillTransparenceGradient");
    CPPUNIT_ASSERT_EQUAL(css::awt::GradientStyle_LINEAR, aGradientxTextFrame.Style);
    //Frame 14
    xTextFrame = getShape(14);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_BITMAP, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Sky"), getProperty<OUString>(xTextFrame, "FillBitmapName"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapPositionOffsetX"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapPositionOffsetY"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapOffsetX"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapOffsetY"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xTextFrame, "FillBitmapTile"));
    //Frame 15
    xTextFrame = getShape(15);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_BITMAP, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Sky"), getProperty<OUString>(xTextFrame, "FillBitmapName"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(45), getProperty<sal_Int32>(xTextFrame, "FillTransparence"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapPositionOffsetX"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapPositionOffsetY"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapOffsetX"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapOffsetY"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xTextFrame, "FillBitmapTile"));
    //Frame 16
    xTextFrame = getShape(16);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_BITMAP, getProperty<drawing::FillStyle>(xTextFrame, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Sky"), getProperty<OUString>(xTextFrame, "FillBitmapName"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapPositionOffsetX"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapPositionOffsetY"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapOffsetX"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xTextFrame, "FillBitmapOffsetY"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xTextFrame, "FillBitmapTile"));
    aGradientxTextFrame = getProperty<awt::Gradient>(xTextFrame, "FillTransparenceGradient");
    CPPUNIT_ASSERT_EQUAL(css::awt::GradientStyle_LINEAR, aGradientxTextFrame.Style);

    if (isExported())
    {
        xmlDocUniquePtr pXmlDoc = parseExport("content.xml");
        // check that there are 3 background-image elements
        assertXPath(pXmlDoc, "//style:style[@style:parent-style-name='Frame' and @style:family='graphic']/style:graphic-properties[@draw:fill='bitmap']/style:background-image[@style:repeat='stretch']", 3);
        // tdf#90640: check that one of them is 55% opaque
        assertXPath(pXmlDoc, "//style:style[@style:parent-style-name='Frame' and @style:family='graphic']/style:graphic-properties[@draw:fill='bitmap' and @fo:background-color='transparent' and @draw:opacity='55%']/style:background-image[@style:repeat='stretch' and @draw:opacity='55%']", 1);
        // tdf#90640: check that one of them is 43% opaque
        // (emulated - hopefully not with rounding errors)
        assertXPath(pXmlDoc, "//style:style[@style:parent-style-name='Frame' and @style:family='graphic']/style:graphic-properties[@draw:fill='bitmap' and @fo:background-color='transparent' and @draw:opacity-name='Transparency_20_1']/style:background-image[@style:repeat='stretch' and @draw:opacity='43%']", 1);
    }
}

DECLARE_SW_ROUNDTRIP_TEST(testSHA1Correct, "sha1_correct.odt", "1012345678901234567890123456789012345678901234567890", Test)
{   // tdf#114939 this has both an affected password as well as content.xml
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    getParagraph(1, "012");
}

DECLARE_SW_ROUNDTRIP_TEST(testSHA1Wrong, "sha1_wrong.odt", "1012345678901234567890123456789012345678901234567890", Test)
{   // tdf#114939 this has both an affected password as well as content.xml
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    getParagraph(1, "012");
}

CPPUNIT_TEST_FIXTURE(Test, testOOoxmlEmbedded)
{
    loadAndReload("oooxml_embedded.sxw");
    uno::Reference<text::XTextEmbeddedObjectsSupplier> xTEOSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xAccess(xTEOSupplier->getEmbeddedObjects());
    uno::Sequence<OUString> aSeq(xAccess->getElementNames());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(4), aSeq.getLength());
    uno::Reference<document::XEmbeddedObjectSupplier> xEOSupplier1(xAccess->getByName("Object1"), uno::UNO_QUERY);
    uno::Reference<lang::XComponent> xObj1(xEOSupplier1->getEmbeddedObject());
    uno::Reference<document::XEmbeddedObjectSupplier> xEOSupplier2(xAccess->getByName("Object2"), uno::UNO_QUERY);
    uno::Reference<lang::XComponent> xObj2(xEOSupplier2->getEmbeddedObject());
    uno::Reference<document::XEmbeddedObjectSupplier> xEOSupplier3(xAccess->getByName("Object3"), uno::UNO_QUERY);
    uno::Reference<lang::XComponent> xObj3(xEOSupplier3->getEmbeddedObject());
    uno::Reference<document::XEmbeddedObjectSupplier> xEOSupplier4(xAccess->getByName("Object4"), uno::UNO_QUERY);
    uno::Reference<lang::XComponent> xObj4(xEOSupplier4->getEmbeddedObject());
    //checking first object
    uno::Reference<document::XStorageBasedDocument> xSBDoc1(xObj1, uno::UNO_QUERY);
    uno::Reference<embed::XStorage> xStorage1(xSBDoc1->getDocumentStorage());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(SOFFICE_FILEFORMAT_8), comphelper::OStorageHelper::GetXStorageFormat(xStorage1));
    //checking second object
    uno::Reference<document::XStorageBasedDocument> xSBDoc2(xObj2, uno::UNO_QUERY);
    uno::Reference<embed::XStorage> xStorage2(xSBDoc2->getDocumentStorage());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(SOFFICE_FILEFORMAT_8), comphelper::OStorageHelper::GetXStorageFormat(xStorage2));
    //checking third object
    uno::Reference<document::XStorageBasedDocument> xSBDoc3(xObj3, uno::UNO_QUERY);
    uno::Reference<embed::XStorage> xStorage3(xSBDoc3->getDocumentStorage());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(SOFFICE_FILEFORMAT_8), comphelper::OStorageHelper::GetXStorageFormat(xStorage3));
    //checking fourth object
    uno::Reference<document::XStorageBasedDocument> xSBDoc4(xObj4, uno::UNO_QUERY);
    uno::Reference<embed::XStorage> xStorage4(xSBDoc4->getDocumentStorage());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(SOFFICE_FILEFORMAT_8), comphelper::OStorageHelper::GetXStorageFormat(xStorage4));
}

DECLARE_ODFEXPORT_TEST(testTdf152710, "tdf152710.odt")
{
    // Without this fix in place, this test would have crash at import time
    CPPUNIT_ASSERT_EQUAL(2, getPages());
}

DECLARE_ODFEXPORT_TEST(testredlineTextFrame, "redlineTextFrame.odt")
{
    CPPUNIT_ASSERT_EQUAL(2, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    //Note this is for a crash test
    //Counting the Number of Frames and checking with the expected count
    uno::Reference<text::XTextFramesSupplier> xTextFramesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xIndexAccess(xTextFramesSupplier->getTextFrames(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), xIndexAccess->getCount());
}

DECLARE_ODFEXPORT_TEST(testTdf107292, "tdf107292.odt")
{
    // tracked deletions at the same position were loaded in reverse order
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    // Without this fix in place, this test would have failed with
    // - Expected: Lorem ipsum dolor sit...
    // - Actual  :  dolor ipsumLorem sit...
    CPPUNIT_ASSERT_EQUAL(OUString("Lorem ipsum dolor sit..."), getParagraph(1)->getString());
}

DECLARE_ODFEXPORT_TEST(testTdf140437, "tdf140437.odt")
{
    // Without the fix in place, the document would have failed to load
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());

    // at least one field should be detected
    CPPUNIT_ASSERT(xFields->hasMoreElements());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf131621)
{
    loadAndReload("tdf131621.ott");
    CPPUNIT_ASSERT_EQUAL(12, getShapes());
    //Crash test, Check number of pages
    CPPUNIT_ASSERT_EQUAL( 1, getPages() );
}

CPPUNIT_TEST_FIXTURE(Test, testTdf135144)
{
    loadAndReload("tdf135144.docx");
    //Crashes at import time after roundtrip
    CPPUNIT_ASSERT_EQUAL(3, getPages());
    CPPUNIT_ASSERT_EQUAL(4, getShapes());
}

DECLARE_ODFEXPORT_TEST(testTdf130950, "tdf130950.odt")
{
    //Crashes at import time
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    CPPUNIT_ASSERT_EQUAL(2, getShapes());
}

DECLARE_ODFEXPORT_TEST(testFdo38244, "fdo38244.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // See ooxmlexport's testFdo38244().

    // Test comment range feature.
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xParaEnumAccess(xTextDocument->getText(), uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xParaEnum = xParaEnumAccess->createEnumeration();
    uno::Reference<container::XEnumerationAccess> xRunEnumAccess(xParaEnum->nextElement(), uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xRunEnum = xRunEnumAccess->createEnumeration();
    xRunEnum->nextElement();
    uno::Reference<beans::XPropertySet> xPropertySet(xRunEnum->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Annotation"), getProperty<OUString>(xPropertySet, "TextPortionType"));
    xRunEnum->nextElement();
    xPropertySet.set(xRunEnum->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AnnotationEnd"), getProperty<OUString>(xPropertySet, "TextPortionType"));

    // Test properties
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());
    xPropertySet.set(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("__Fieldmark__4_1833023242"), getProperty<OUString>(xPropertySet, "Name"));
    CPPUNIT_ASSERT_EQUAL(OUString("M"), getProperty<OUString>(xPropertySet, "Initials"));
}

CPPUNIT_TEST_FIXTURE(Test, testSenderInitials)
{
    loadAndReload("sender-initials.fodt");
    // Test sender-initial properties (both annotation metadata and text field)
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());
    // first 3 are annotations, last 2 are text fields
    for (unsigned i = 0; i < 3; ++i)
    {
        uno::Reference<beans::XPropertySet> xPropertySet(xFields->nextElement(), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(OUString("I"), getProperty<OUString>(xPropertySet, "Initials"));
    }
    for (unsigned i = 0; i < 2; ++i)
    {
        uno::Reference<beans::XPropertySet> xPropertySet(xFields->nextElement(), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xPropertySet, "IsFixed"));
        CPPUNIT_ASSERT_EQUAL(OUString("I"), getProperty<OUString>(xPropertySet, "Content"));
    }
}

DECLARE_ODFEXPORT_TEST(testResolvedComment, "resolved-comment.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());
    uno::Reference<beans::XPropertySet> xPropertySet(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xPropertySet, "Resolved"));
    xPropertySet.set(xFields->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(false, getProperty<bool>(xPropertySet, "Resolved"));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf92379)
{
    loadAndReload("tdf92379.fodt");
    // frame style fo:background-color was not imported
    uno::Reference<container::XNameAccess> xStyles(getStyles("FrameStyles"));
    uno::Reference<beans::XPropertySet> xStyle(xStyles->getByName("encarts"),
            uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(Color(0xffcc99), getProperty<Color>(xStyle, "BackColorRGB"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), getProperty<sal_Int32>(xStyle, "BackColorTransparency"));
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_SOLID, getProperty<drawing::FillStyle>(xStyle, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(Color(0xffcc99), getProperty<Color>(xStyle, "FillColor"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(0), getProperty<sal_Int16>(xStyle, "FillTransparence"));

    uno::Reference<beans::XPropertySet> xFrameStyle2(xStyles->getByName("Untitled1"),
            uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(COL_WHITE, getProperty<Color>(xFrameStyle2, "BackColorRGB"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xFrameStyle2, "BackTransparent"));
    CPPUNIT_ASSERT_EQUAL(sal_Int32(100), getProperty<sal_Int32>(xFrameStyle2, "BackColorTransparency"));
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_NONE, getProperty<drawing::FillStyle>(xFrameStyle2, "FillStyle"));
// unfortunately this is actually the pool default value, which would be hard to fix - but it isn't a problem because style is NONE
//    CPPUNIT_ASSERT_EQUAL(sal_Int32(0xffffff), getProperty<sal_Int32>(xFrameStyle2, "FillColor"));
//    CPPUNIT_ASSERT_EQUAL(sal_Int16(100), getProperty<sal_Int16>(xFrameStyle2, "FillTransparence"));

    if (isExported())
    {
        xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");
        // check that fo:background-color attribute is exported properly
        assertXPath(pXmlDoc, "//style:style[@style:family='graphic' and @style:name='encarts']/style:graphic-properties[@fo:background-color='#ffcc99']", 1);
        assertXPath(pXmlDoc, "//style:style[@style:family='graphic' and @style:name='Untitled1']/style:graphic-properties[@fo:background-color='transparent']", 1);
    }

    // paragraph style fo:background-color was wrongly inherited despite being
    // overridden in derived style
    uno::Reference<container::XNameAccess> xParaStyles(getStyles("ParagraphStyles"));
    uno::Reference<beans::XPropertySet> xStyle1(xParaStyles->getByName(
            "Titre Avis expert"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(Color(0x661900), getProperty<Color>(xStyle1, "ParaBackColor"));
    CPPUNIT_ASSERT_EQUAL(false, getProperty<bool>(xStyle1, "ParaBackTransparent"));
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_SOLID, getProperty<drawing::FillStyle>(xStyle1, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(Color(0x661900), getProperty<Color>(xStyle1, "FillColor"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(0), getProperty<sal_Int16>(xStyle1, "FillTransparence"));
    CPPUNIT_ASSERT_EQUAL(COL_WHITE, getProperty<Color>(xStyle1, "CharColor"));

    uno::Reference<beans::XPropertySet> xStyle2(xParaStyles->getByName(
            "Avis expert questions"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(COL_TRANSPARENT, getProperty<Color>(xStyle2, "ParaBackColor"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xStyle2, "ParaBackTransparent"));
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_NONE, getProperty<drawing::FillStyle>(xStyle2, "FillStyle"));
// unfortunately this is actually the pool default value, which would be hard to fix - but it isn't a problem because style is NONE
//    CPPUNIT_ASSERT_EQUAL(sal_Int32(0xffffff), getProperty<sal_Int32>(xStyle2, "FillColor"));
//    CPPUNIT_ASSERT_EQUAL(sal_Int16(100), getProperty<sal_Int16>(xStyle2, "FillTransparence"));
    CPPUNIT_ASSERT_EQUAL(Color(0x661900), getProperty<Color>(xStyle2, "CharColor"));

    uno::Reference<beans::XPropertySet> xStyle31(xParaStyles->getByName(
            "avis expert questions non cadres"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(Color(0x801900), getProperty<Color>(xStyle31, "ParaBackColor"));
    CPPUNIT_ASSERT_EQUAL(false, getProperty<bool>(xStyle31, "ParaBackTransparent"));
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_SOLID, getProperty<drawing::FillStyle>(xStyle31, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(Color(0x801900), getProperty<Color>(xStyle31, "FillColor"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(0), getProperty<sal_Int16>(xStyle31, "FillTransparence"));
    CPPUNIT_ASSERT_EQUAL(Color(0x661900), getProperty<Color>(xStyle31, "CharColor"));

    uno::Reference<beans::XPropertySet> xStyle32(xParaStyles->getByName(
            "Avis expert rXponses"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(COL_TRANSPARENT, getProperty<Color>(xStyle32, "ParaBackColor"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xStyle32, "ParaBackTransparent"));
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_NONE, getProperty<drawing::FillStyle>(xStyle32, "FillStyle"));
// unfortunately this is actually the pool default value, which would be hard to fix - but it isn't a problem because style is NONE
//    CPPUNIT_ASSERT_EQUAL(sal_Int32(0xffffff), getProperty<sal_Int32>(xStyle32, "FillColor"));
//    CPPUNIT_ASSERT_EQUAL(sal_Int16(100), getProperty<sal_Int16>(xStyle32, "FillTransparence"));
    CPPUNIT_ASSERT_EQUAL(Color(0x461900), getProperty<Color>(xStyle32, "CharColor"));

    if (isExported())
    {
        xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");
        // check that fo:background-color attribute is exported properly
        assertXPath(pXmlDoc, "//style:style[@style:family='paragraph' and @style:display-name='Titre Avis expert']/style:paragraph-properties[@fo:background-color='#661900']", 1);
        assertXPath(pXmlDoc, "//style:style[@style:family='paragraph' and @style:display-name='Avis expert questions']/style:paragraph-properties[@fo:background-color='transparent']", 1);
        assertXPath(pXmlDoc, "//style:style[@style:family='paragraph' and @style:display-name='avis expert questions non cadres']/style:paragraph-properties[@fo:background-color='#801900']", 1);
        assertXPath(pXmlDoc, "//style:style[@style:family='paragraph' and @style:display-name='Avis expert rXponses']/style:paragraph-properties[@fo:background-color='transparent']", 1);
    }
}

DECLARE_ODFEXPORT_TEST(testFdo79358, "fdo79358.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // the boolean properties of the index were not exported properly
    uno::Reference<text::XDocumentIndexesSupplier> xIndexSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xIndexes = xIndexSupplier->getDocumentIndexes();
    uno::Reference<text::XDocumentIndex> xTOCIndex(xIndexes->getByIndex(0), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xTOCProps(xTOCIndex, uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(false, getProperty<bool>(xTOCProps, "CreateFromOutline"));
    CPPUNIT_ASSERT_EQUAL(false, getProperty<bool>(xTOCProps, "CreateFromMarks"));
    CPPUNIT_ASSERT_EQUAL(true, getProperty<bool>(xTOCProps, "CreateFromLevelParagraphStyles"));
    // check that the source styles are preserved too while at it
    uno::Reference<container::XIndexReplace> xLevels(
        getProperty< uno::Reference<container::XIndexReplace> >(xTOCProps,
            "LevelParagraphStyles"));
    uno::Sequence<OUString> seq { "Heading" };
    CPPUNIT_ASSERT_EQUAL(uno::Any(seq), xLevels->getByIndex(1));
    CPPUNIT_ASSERT_EQUAL(uno::Any(uno::Sequence<OUString>()), xLevels->getByIndex(2));
}

DECLARE_ODFEXPORT_TEST(testFirstHeaderFooter, "first-header-footer.odt")
{
    CPPUNIT_ASSERT_EQUAL(6, getPages());
    // Test import and export of the header-first token.

    // The document has 6 pages, two page styles for the first and second half of pages.
    CPPUNIT_ASSERT_EQUAL(OUString("First header"),  parseDump("/root/page[1]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("First footer"),  parseDump("/root/page[1]/footer/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Left header"),   parseDump("/root/page[2]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Left footer"),   parseDump("/root/page[2]/footer/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Right header"),  parseDump("/root/page[3]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Right footer"),  parseDump("/root/page[3]/footer/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("First header2"), parseDump("/root/page[4]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("First footer2"), parseDump("/root/page[4]/footer/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Right header2"), parseDump("/root/page[5]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Right footer2"), parseDump("/root/page[5]/footer/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Left header2"),  parseDump("/root/page[6]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Left footer2"),  parseDump("/root/page[6]/footer/txt/text()"));
}

DECLARE_ODFEXPORT_TEST(testTextframeGradient, "textframe-gradient.odt")
{
    CPPUNIT_ASSERT_EQUAL(2, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<text::XTextFramesSupplier> xTextFramesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xIndexAccess(xTextFramesSupplier->getTextFrames(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2), xIndexAccess->getCount());

    uno::Reference<beans::XPropertySet> xFrame(xIndexAccess->getByIndex(0), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_GRADIENT, getProperty<drawing::FillStyle>(xFrame, "FillStyle"));
    awt::Gradient2 aGradient = getProperty<awt::Gradient2>(xFrame, "FillGradient");

    // MCGR: Use the completely imported gradient to check for correctness
    basegfx::BColorStops aColorStops = model::gradient::getColorStopsFromUno(aGradient.ColorStops);

    CPPUNIT_ASSERT_EQUAL(size_t(2), aColorStops.size());
    CPPUNIT_ASSERT(basegfx::fTools::equal(aColorStops[0].getStopOffset(), 0.0));
    CPPUNIT_ASSERT_EQUAL(Color(0xc0504d), Color(aColorStops[0].getStopColor()));
    CPPUNIT_ASSERT(basegfx::fTools::equal(aColorStops[1].getStopOffset(), 1.0));
    CPPUNIT_ASSERT_EQUAL(Color(0xd99594), Color(aColorStops[1].getStopColor()));
    CPPUNIT_ASSERT_EQUAL(awt::GradientStyle_AXIAL, aGradient.Style);

    xFrame.set(xIndexAccess->getByIndex(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_GRADIENT, getProperty<drawing::FillStyle>(xFrame, "FillStyle"));
    aGradient = getProperty<awt::Gradient2>(xFrame, "FillGradient");

    // MCGR: Use the completely imported gradient to check for correctness
    aColorStops = model::gradient::getColorStopsFromUno(aGradient.ColorStops);

    CPPUNIT_ASSERT_EQUAL(size_t(2), aColorStops.size());
    CPPUNIT_ASSERT(basegfx::fTools::equal(aColorStops[0].getStopOffset(), 0.0));
    CPPUNIT_ASSERT_EQUAL(Color(0x000000), Color(aColorStops[0].getStopColor()));
    CPPUNIT_ASSERT(basegfx::fTools::equal(aColorStops[1].getStopOffset(), 1.0));
    CPPUNIT_ASSERT_EQUAL(Color(0x666666), Color(aColorStops[1].getStopColor()));
    CPPUNIT_ASSERT_EQUAL(awt::GradientStyle_AXIAL, aGradient.Style);
}

CPPUNIT_TEST_FIXTURE(Test, testDuplicateCrossRefHeadingBookmark)
{
    loadAndReload("CrossRefHeadingBookmark.fodt");
    // the file contains invalid duplicate heading cross reference bookmarks
    // but we have to round trip them, tdf#94804

    uno::Reference<text::XBookmarksSupplier> xBookmarksSupplier(mxComponent,
        uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xBookmarks =
        xBookmarksSupplier->getBookmarks();
    uno::Reference<text::XTextContent> xBookmark1(
        xBookmarks->getByName("__RefHeading__8284_1826734303"), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xBookmark1.is());
    CPPUNIT_ASSERT_THROW(xBookmarks->getByName("__RefHeading__1673_25705824"), container::NoSuchElementException);

    uno::Reference<text::XTextFieldsSupplier> xTextFieldsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<util::XRefreshable>(xTextFieldsSupplier->getTextFields(), uno::UNO_QUERY_THROW)->refresh();

    uno::Reference<container::XEnumerationAccess> xFieldsAccess(xTextFieldsSupplier->getTextFields());
    uno::Reference<container::XEnumeration> xFields(xFieldsAccess->createEnumeration());
    uno::Any aField1 = xFields->nextElement();
    uno::Reference<text::XTextField> xField1(aField1, uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("1.1"), xField1->getPresentation(false));
    uno::Any aField2 = xFields->nextElement();
    uno::Reference<text::XTextField> xField2(aField2, uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("1.1"), xField2->getPresentation(false));
}

DECLARE_ODFEXPORT_TEST(testFdo60769, "fdo60769.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Test multi-paragraph comment range feature.
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xParaEnumAccess(xTextDocument->getText(), uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xParaEnum = xParaEnumAccess->createEnumeration();
    uno::Reference<container::XEnumerationAccess> xRunEnumAccess(xParaEnum->nextElement(), uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xRunEnum = xRunEnumAccess->createEnumeration();
    while (xRunEnum->hasMoreElements())
    {
        uno::Reference<beans::XPropertySet> xPropertySet(xRunEnum->nextElement(), uno::UNO_QUERY);
        OUString aType =  getProperty<OUString>(xPropertySet, "TextPortionType");
        // First paragraph: no field end, no anchor
        CPPUNIT_ASSERT(aType == "Text" || aType == "Annotation");
    }

    xRunEnumAccess.set(xParaEnum->nextElement(), uno::UNO_QUERY);
    while (xRunEnum->hasMoreElements())
    {
        uno::Reference<beans::XPropertySet> xPropertySet(xRunEnum->nextElement(), uno::UNO_QUERY);
        OUString aType =  getProperty<OUString>(xPropertySet, "TextPortionType");
        // Second paragraph: no field start
        CPPUNIT_ASSERT(aType == "Text" || aType == "AnnotationEnd");
    }
}

DECLARE_ODFEXPORT_TEST(testTdf115815, "tdf115815.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Test comment range feature on tracked deletion.
    uno::Reference<text::XTextDocument> xTextDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xParaEnumAccess(xTextDocument->getText(), uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xParaEnum = xParaEnumAccess->createEnumeration();
    uno::Reference<container::XEnumerationAccess> xRunEnumAccess(xParaEnum->nextElement(), uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xRunEnum = xRunEnumAccess->createEnumeration();
    bool bAnnotationStart = false;
    bool bBeforeAnnotation = true;
    OUString sTextBeforeAnnotation;
    while (xRunEnum->hasMoreElements())
    {
        uno::Reference<beans::XPropertySet> xPropertySet(xRunEnum->nextElement(), uno::UNO_QUERY);
        OUString aType = getProperty<OUString>(xPropertySet, "TextPortionType");
        // there is no AnnotationEnd with preceding AnnotationStart,
        // i.e. annotation with lost range
        CPPUNIT_ASSERT(aType != "AnnotationEnd" || !bAnnotationStart);

        bAnnotationStart = (aType == "Annotation");

        // collect paragraph text before the first annotation
        if (bBeforeAnnotation)
        {
            if (bAnnotationStart)
                bBeforeAnnotation = false;
            else if (aType == "Text")
            {
                uno::Reference<text::XTextRange> xRun(xPropertySet, uno::UNO_QUERY);
                sTextBeforeAnnotation += xRun->getString();
            }
        }
    }

    // This was "Lorem ipsum" (collapsed annotation range)
    CPPUNIT_ASSERT_EQUAL(OUString("Lorem "), sTextBeforeAnnotation);
}

CPPUNIT_TEST_FIXTURE(Test, testFdo58949)
{
    comphelper::ScopeGuard g([]() {
        std::shared_ptr<comphelper::ConfigurationChanges> pBatch(
            comphelper::ConfigurationChanges::create());
        officecfg::Office::Common::Filter::Microsoft::Import::MathTypeToMath::set(true, pBatch);
        pBatch->commit();
    });

    std::shared_ptr<comphelper::ConfigurationChanges> pBatch(
        comphelper::ConfigurationChanges::create());
    officecfg::Office::Common::Filter::Microsoft::Import::MathTypeToMath::set(false, pBatch);
    pBatch->commit();
    loadAndReload("fdo58949.docx");

    /*
     * The problem was that the exporter didn't insert "Obj102" to the
     * resulting zip file. No idea how to check for "broken" (missing OLE data
     * and replacement image) OLE objects using UNO, so we'll check the zip file directly.
     */

    save("writer8");

    uno::Sequence<uno::Any> aArgs{ uno::Any(maTempFile.GetURL()) };
    uno::Reference<container::XNameAccess> xNameAccess(m_xSFactory->createInstanceWithArguments("com.sun.star.packages.zip.ZipFileAccess", aArgs), uno::UNO_QUERY);
    const css::uno::Sequence<OUString> aNames(xNameAccess->getElementNames());
    // The exported document must have three objects named ObjNNN. The names are assigned in
    // OLEHandler::copyOLEOStream using a static counter, and actual numbers depend on previous
    // tests; so just count the matching names here.
    int nMatches = 0;
    for (const OUString& sName : aNames)
    {
        OUString sRest;
        if (sName.startsWith("Obj", &sRest))
        {
            // all following characters must be decimal digits; minimal value is 100
            bool bMatch = sRest.getLength() >= 3
                          && std::all_of(sRest.getStr(), sRest.getStr() + sRest.getLength(),
                                 [](sal_Unicode ch) { return ch >= '0' && ch <= '9'; });
            if (bMatch)
                ++nMatches;
        }
    }
    CPPUNIT_ASSERT_EQUAL(3, nMatches);
}

CPPUNIT_TEST_FIXTURE(Test, testTdf134987)
{
    loadAndReload("tdf134987.docx");
    uno::Reference<text::XTextEmbeddedObjectsSupplier> xTEOSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xAccess(xTEOSupplier->getEmbeddedObjects());
    uno::Sequence<OUString> aSeq(xAccess->getElementNames());
    CPPUNIT_ASSERT_EQUAL(sal_Int32(3), aSeq.getLength());

    OUString aMediaType;
    // checking first object (formula)
    {
        uno::Reference<document::XEmbeddedObjectSupplier> xEOSupplier(xAccess->getByName("Object1"), uno::UNO_QUERY);
        uno::Reference<lang::XComponent> xObj(xEOSupplier->getEmbeddedObject());
        CPPUNIT_ASSERT(xObj.is());

        uno::Reference<document::XStorageBasedDocument> xSBDoc(xObj, uno::UNO_QUERY);
        uno::Reference<embed::XStorage> xStorage(xSBDoc->getDocumentStorage());
        CPPUNIT_ASSERT(xStorage.is());

        uno::Reference< beans::XPropertySet > xStorProps(xStorage, uno::UNO_QUERY_THROW);
        CPPUNIT_ASSERT(xStorProps->getPropertyValue("MediaType") >>= aMediaType);
        CPPUNIT_ASSERT(aMediaType.equalsIgnoreAsciiCase(MIMETYPE_OASIS_OPENDOCUMENT_FORMULA_ASCII));
    }
    // checking second object (chart)
    {
        uno::Reference<document::XEmbeddedObjectSupplier> xEOSupplier(xAccess->getByName("Diagram 1"), uno::UNO_QUERY);
        uno::Reference<lang::XComponent> xObj(xEOSupplier->getEmbeddedObject());
        CPPUNIT_ASSERT(xObj.is());

        uno::Reference<document::XStorageBasedDocument> xSBDoc(xObj, uno::UNO_QUERY);
        uno::Reference<embed::XStorage> xStorage(xSBDoc->getDocumentStorage());
        CPPUNIT_ASSERT(xStorage.is());

        uno::Reference< beans::XPropertySet > xStorProps(xStorage, uno::UNO_QUERY_THROW);
        CPPUNIT_ASSERT(xStorProps->getPropertyValue("MediaType") >>= aMediaType);
        CPPUNIT_ASSERT(aMediaType.equalsIgnoreAsciiCase(MIMETYPE_OASIS_OPENDOCUMENT_CHART_ASCII));
    }
    // checking third object (chart)
    {
        uno::Reference<document::XEmbeddedObjectSupplier> xEOSupplier(xAccess->getByName("Diagram 2"), uno::UNO_QUERY);
        uno::Reference<lang::XComponent> xObj(xEOSupplier->getEmbeddedObject());
        CPPUNIT_ASSERT(xObj.is());

        uno::Reference<document::XStorageBasedDocument> xSBDoc(xObj, uno::UNO_QUERY);
        uno::Reference<embed::XStorage> xStorage(xSBDoc->getDocumentStorage());
        CPPUNIT_ASSERT(xStorage.is());

        uno::Reference< beans::XPropertySet > xStorProps(xStorage, uno::UNO_QUERY_THROW);
        CPPUNIT_ASSERT(xStorProps->getPropertyValue("MediaType") >>= aMediaType);
        CPPUNIT_ASSERT(aMediaType.equalsIgnoreAsciiCase(MIMETYPE_OASIS_OPENDOCUMENT_CHART_ASCII));
    }
}

DECLARE_ODFEXPORT_TEST(testStylePageNumber, "ooo321_stylepagenumber.odt")
{
    CPPUNIT_ASSERT_EQUAL(5, getPages());
    uno::Reference<text::XTextContent> xTable1(getParagraphOrTable(1));
// actually no break attribute is written in this case
//    CPPUNIT_ASSERT_EQUAL(style::BreakType_PAGE_BEFORE, getProperty<style::BreakType>(xTable1, "BreakType"));
    CPPUNIT_ASSERT_EQUAL(OUString("Left Page"), getProperty<OUString>(xTable1, "PageDescName"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), getProperty<sal_Int16>(xTable1, "PageNumberOffset"));

    uno::Reference<text::XTextContent> xPara1(getParagraphOrTable(2));
    CPPUNIT_ASSERT_EQUAL(OUString("Right Page"), getProperty<OUString>(xPara1, "PageDescName"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), getProperty<sal_Int16>(xPara1, "PageNumberOffset"));

    // i#114163 tdf#77111: OOo < 3.3 bug, it wrote "auto" as "0" for tables
    uno::Reference<beans::XPropertySet> xTable0(getParagraphOrTable(3), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Left Page"), getProperty<OUString>(xTable0, "PageDescName"));
    CPPUNIT_ASSERT_EQUAL(uno::Any(), xTable0->getPropertyValue("PageNumberOffset"));

    uno::Reference<beans::XPropertySet> xPara0(getParagraphOrTable(4), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Right Page"), getProperty<OUString>(xPara0, "PageDescName"));
    CPPUNIT_ASSERT_EQUAL(uno::Any(), xPara0->getPropertyValue("PageNumberOffset"));

    uno::Reference<container::XNameAccess> xParaStyles = getStyles("ParagraphStyles");
    uno::Reference<beans::XPropertySet> xStyle1(xParaStyles->getByName("stylewithbreak1"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Right Page"), getProperty<OUString>(xStyle1, "PageDescName"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), getProperty<sal_Int16>(xStyle1, "PageNumberOffset"));

    uno::Reference<beans::XPropertySet> xStyle0(xParaStyles->getByName("stylewithbreak0"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("First Page"), getProperty<OUString>(xStyle0, "PageDescName"));
    CPPUNIT_ASSERT_EQUAL(uno::Any(), xStyle0->getPropertyValue("PageNumberOffset"));
}

DECLARE_ODFEXPORT_TEST(testCharacterBorder, "charborder.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Make sure paragraph and character attributes don't interfere
    // First paragraph has a paragraph border and a character border included by the paragraph style

    // Paragraph border of first paragraph
    {
        const table::BorderLine2 aFirstParTopBorder(0x6666FF,2,26,26,7,55);
        const sal_Int32 aFirstParTopPadding(150);
        uno::Reference<beans::XPropertySet> xSet(getParagraph(1), uno::UNO_QUERY);

        // Top border
        CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParTopBorder, getProperty<table::BorderLine2>(xSet,"TopBorder"));
        CPPUNIT_ASSERT_EQUAL(aFirstParTopPadding, getProperty<sal_Int32>(xSet,"TopBorderDistance"));

        // Bottom border (same as top border)
        CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParTopBorder, getProperty<table::BorderLine2>(xSet,"BottomBorder"));
        CPPUNIT_ASSERT_EQUAL(aFirstParTopPadding, getProperty<sal_Int32>(xSet,"BottomBorderDistance"));

        // Left border (same as top border)
        CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParTopBorder, getProperty<table::BorderLine2>(xSet,"LeftBorder"));
        CPPUNIT_ASSERT_EQUAL(aFirstParTopPadding, getProperty<sal_Int32>(xSet,"LeftBorderDistance"));

        // Right border (same as top border)
        CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParTopBorder, getProperty<table::BorderLine2>(xSet,"RightBorder"));
        CPPUNIT_ASSERT_EQUAL(aFirstParTopPadding, getProperty<sal_Int32>(xSet,"RightBorderDistance"));

        // Shadow
        const table::ShadowFormat aShadow = getProperty<table::ShadowFormat>(xSet,"ParaShadowFormat");
        CPPUNIT_ASSERT_EQUAL(COL_BLACK, Color(ColorTransparency, aShadow.Color));
        CPPUNIT_ASSERT_EQUAL(false, static_cast<bool>(aShadow.IsTransparent));
        CPPUNIT_ASSERT_EQUAL(table::ShadowLocation(0), aShadow.Location);
        CPPUNIT_ASSERT_EQUAL(sal_Int16(0), aShadow.ShadowWidth);
    }

    // Character border for first paragraph
    {
        const table::BorderLine2 aFirstParCharTopBorder(0xFF3333,0,37,0,2,37);
        const sal_Int32 aFirstParCharTopPadding(450);
        uno::Reference<beans::XPropertySet> xSet(getParagraph(1), uno::UNO_QUERY);

        // Top border
        CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParCharTopBorder, getProperty<table::BorderLine2>(xSet,"CharTopBorder"));
        CPPUNIT_ASSERT_EQUAL(aFirstParCharTopPadding, getProperty<sal_Int32>(xSet,"CharTopBorderDistance"));

        // Bottom border (same as top border)
        CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParCharTopBorder, getProperty<table::BorderLine2>(xSet,"CharBottomBorder"));
        CPPUNIT_ASSERT_EQUAL(aFirstParCharTopPadding, getProperty<sal_Int32>(xSet,"CharBottomBorderDistance"));

        // Left border (same as top border)
        CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParCharTopBorder, getProperty<table::BorderLine2>(xSet,"CharLeftBorder"));
        CPPUNIT_ASSERT_EQUAL(aFirstParCharTopPadding, getProperty<sal_Int32>(xSet,"CharLeftBorderDistance"));

        // Right border (same as top border)
        CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParCharTopBorder, getProperty<table::BorderLine2>(xSet,"CharRightBorder"));
        CPPUNIT_ASSERT_EQUAL(aFirstParCharTopPadding, getProperty<sal_Int32>(xSet,"CharRightBorderDistance"));

        // Shadow
        const table::ShadowFormat aShadow = getProperty<table::ShadowFormat>(xSet,"CharShadowFormat");
        CPPUNIT_ASSERT_EQUAL(Color(0xFF3333), Color(ColorTransparency, aShadow.Color));
        CPPUNIT_ASSERT_EQUAL(false, static_cast<bool>(aShadow.IsTransparent));
        CPPUNIT_ASSERT_EQUAL(table::ShadowLocation(2), aShadow.Location);
        CPPUNIT_ASSERT_EQUAL(sal_Int16(280), aShadow.ShadowWidth);

        // Check autostyle
        {
            uno::Reference< style::XAutoStyleFamily > xAutoStyleFamily(getAutoStyles("ParagraphStyles"));
            uno::Reference < container::XEnumeration > xAutoStylesEnum( xAutoStyleFamily->createEnumeration() );
            CPPUNIT_ASSERT_EQUAL(true, static_cast<bool>(xAutoStylesEnum->hasMoreElements()));

            // Top border
            CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParCharTopBorder, getProperty<table::BorderLine2>(xSet,"CharTopBorder"));
            CPPUNIT_ASSERT_EQUAL(aFirstParCharTopPadding, getProperty<sal_Int32>(xSet,"CharTopBorderDistance"));

            // Bottom border
            CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParCharTopBorder, getProperty<table::BorderLine2>(xSet,"CharBottomBorder"));
            CPPUNIT_ASSERT_EQUAL(aFirstParCharTopPadding, getProperty<sal_Int32>(xSet,"CharBottomBorderDistance"));

            // Left border
            CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParCharTopBorder, getProperty<table::BorderLine2>(xSet,"CharLeftBorder"));
            CPPUNIT_ASSERT_EQUAL(aFirstParCharTopPadding, getProperty<sal_Int32>(xSet,"CharLeftBorderDistance"));

            // Right border
            CPPUNIT_ASSERT_BORDER_EQUAL(aFirstParCharTopBorder, getProperty<table::BorderLine2>(xSet,"CharRightBorder"));
            CPPUNIT_ASSERT_EQUAL(aFirstParCharTopPadding, getProperty<sal_Int32>(xSet,"CharRightBorderDistance"));
        }
    }

    // Second paragraph's second text portion has a character style named CharDiffBor
    // This style includes border with different sides
    {

        table::BorderLine2 aBorderArray[4] =
        {
            table::BorderLine2(0xFF3333,0,37,0,14,37),     // Top (fine dashed line)
            table::BorderLine2(0x99FF66,26,26,53,11,106),  // Bottom
            table::BorderLine2(0x6666FF,9,26,9,12,71),     // Left
            table::BorderLine2(0,0,0,0,table::BorderLineStyle::NONE,0) // Right
        };

        sal_Int32 aDistances[4] = { 400 /*Top*/, 300 /*Bottom*/, 250 /*Left*/, 0 /*Right*/ };

        // Get second text portion of second paragraph
        uno::Reference < beans::XPropertySet > xSet( getRun(getParagraph(2),2), uno::UNO_QUERY );

        // Top border
        CPPUNIT_ASSERT_BORDER_EQUAL(aBorderArray[0], getProperty<table::BorderLine2>(xSet,"CharTopBorder"));
        CPPUNIT_ASSERT_EQUAL(aDistances[0], getProperty<sal_Int32>(xSet,"CharTopBorderDistance"));

        // Bottom border
        CPPUNIT_ASSERT_BORDER_EQUAL(aBorderArray[1], getProperty<table::BorderLine2>(xSet,"CharBottomBorder"));
        CPPUNIT_ASSERT_EQUAL(aDistances[1], getProperty<sal_Int32>(xSet,"CharBottomBorderDistance"));

        // Left border
        CPPUNIT_ASSERT_BORDER_EQUAL(aBorderArray[2], getProperty<table::BorderLine2>(xSet,"CharLeftBorder"));
        CPPUNIT_ASSERT_EQUAL(aDistances[2], getProperty<sal_Int32>(xSet,"CharLeftBorderDistance"));

        // Right border
        CPPUNIT_ASSERT_BORDER_EQUAL(aBorderArray[3], getProperty<table::BorderLine2>(xSet,"CharRightBorder"));
        CPPUNIT_ASSERT_EQUAL(aDistances[3], getProperty<sal_Int32>(xSet,"CharRightBorderDistance"));

        // Shadow
        const table::ShadowFormat aShadow = getProperty<table::ShadowFormat>(xSet,"CharShadowFormat");
        CPPUNIT_ASSERT_EQUAL(COL_BLACK, Color(ColorTransparency, aShadow.Color));
        CPPUNIT_ASSERT_EQUAL(false, static_cast<bool>(aShadow.IsTransparent));
        CPPUNIT_ASSERT_EQUAL(table::ShadowLocation(3), aShadow.Location);
        CPPUNIT_ASSERT_EQUAL(sal_Int16(79), aShadow.ShadowWidth);

        // Check character style
        {
            uno::Reference< container::XNameAccess > xStyleFamily = getStyles("CharacterStyles");
            uno::Reference < beans::XPropertySet > xStyleSet(xStyleFamily->getByName("CharDiffBor"), uno::UNO_QUERY);

            // Top border
            CPPUNIT_ASSERT_BORDER_EQUAL(aBorderArray[0], getProperty<table::BorderLine2>(xStyleSet,"CharTopBorder"));
            CPPUNIT_ASSERT_EQUAL(aDistances[0], getProperty<sal_Int32>(xStyleSet,"CharTopBorderDistance"));

            // Bottom border
            CPPUNIT_ASSERT_BORDER_EQUAL(aBorderArray[1], getProperty<table::BorderLine2>(xStyleSet,"CharBottomBorder"));
            CPPUNIT_ASSERT_EQUAL(aDistances[1], getProperty<sal_Int32>(xStyleSet,"CharBottomBorderDistance"));

            // Left border
            CPPUNIT_ASSERT_BORDER_EQUAL(aBorderArray[2], getProperty<table::BorderLine2>(xStyleSet,"CharLeftBorder"));
            CPPUNIT_ASSERT_EQUAL(aDistances[2], getProperty<sal_Int32>(xStyleSet,"CharLeftBorderDistance"));

            // Right border
            CPPUNIT_ASSERT_BORDER_EQUAL(aBorderArray[3], getProperty<table::BorderLine2>(xStyleSet,"CharRightBorder"));
            CPPUNIT_ASSERT_EQUAL(aDistances[3], getProperty<sal_Int32>(xStyleSet,"CharRightBorderDistance"));
        }
    }
}

CPPUNIT_TEST_FIXTURE(Test, testProtectionKey)
{
    loadAndReload("protection-key.fodt");
    OUString constexpr password(u"1012345678901234567890123456789012345678901234567890"_ustr);

    // check 1 invalid OOo legacy password and 3 valid ODF 1.2 passwords
    uno::Reference<text::XTextSectionsSupplier> xTextSectionsSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XIndexAccess> xSections(xTextSectionsSupplier->getTextSections(), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xSect0(xSections->getByIndex(0), uno::UNO_QUERY);
    uno::Sequence<sal_Int8> const key0(getProperty<uno::Sequence<sal_Int8>>(xSect0, "ProtectionKey"));
    CPPUNIT_ASSERT(SvPasswordHelper::CompareHashPassword(key0, password));
    uno::Reference<beans::XPropertySet> xSect1(xSections->getByIndex(1), uno::UNO_QUERY);
    uno::Sequence<sal_Int8> const key1(getProperty<uno::Sequence<sal_Int8>>(xSect1, "ProtectionKey"));
    CPPUNIT_ASSERT(SvPasswordHelper::CompareHashPassword(key1, password));
    uno::Reference<beans::XPropertySet> xSect2(xSections->getByIndex(2), uno::UNO_QUERY);
    uno::Sequence<sal_Int8> const key2(getProperty<uno::Sequence<sal_Int8>>(xSect2, "ProtectionKey"));
    CPPUNIT_ASSERT(SvPasswordHelper::CompareHashPassword(key2, password));
    uno::Reference<beans::XPropertySet> xSect3(xSections->getByIndex(3), uno::UNO_QUERY);
    uno::Sequence<sal_Int8> const key3(getProperty<uno::Sequence<sal_Int8>>(xSect3, "ProtectionKey"));
    CPPUNIT_ASSERT(SvPasswordHelper::CompareHashPassword(key3, password));

    // we can't assume that the user entered the password; check that we
    // round-trip the password as-is
    if (isExported())
    {
        xmlDocUniquePtr pXmlDoc = parseExport("content.xml");
        assertXPath(pXmlDoc, "//text:section[@text:name='Section0' and @text:protected='true' and @text:protection-key='vbnhxyBKtPHCA1wB21zG1Oha8ZA=']");
        assertXPath(pXmlDoc, "//text:section[@text:name='Section1' and @text:protected='true' and @text:protection-key='nLHas0RIwepGDaH4c2hpyIUvIS8=']");
        assertXPath(pXmlDoc, "//text:section[@text:name='Section2' and @text:protected='true' and @text:protection-key-digest-algorithm='http://www.w3.org/2000/09/xmldsig#sha256' and @text:protection-key='1tnJohagR2T0yF/v69hLPuumSTsj32CumW97nkKGuSQ=']");
        assertXPath(pXmlDoc, "//text:section[@text:name='Section3' and @text:protected='true' and @text:protection-key-digest-algorithm='http://www.w3.org/2000/09/xmldsig#sha256' and @text:protection-key='1tnJohagR2T0yF/v69hLPuumSTsj32CumW97nkKGuSQ=']");
    }
}

CPPUNIT_TEST_FIXTURE(Test, testTdf128188)
{
    loadAndReload("footnote-collect-at-end-of-section.fodt");
    SwDoc *const pDoc = dynamic_cast<SwXTextDocument&>(*mxComponent).GetDocShell()->GetDoc();
    CPPUNIT_ASSERT(pDoc);
    SwFootnoteIdxs const& rFootnotes(pDoc->GetFootnoteIdxs());
    // Section1
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(1), rFootnotes[0]->GetFootnote().GetNumber());
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(1), rFootnotes[0]->GetFootnote().GetNumberRLHidden());
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(2), rFootnotes[1]->GetFootnote().GetNumber());
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(2), rFootnotes[1]->GetFootnote().GetNumberRLHidden());
    // Section2
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(1), rFootnotes[2]->GetFootnote().GetNumber());
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(1), rFootnotes[2]->GetFootnote().GetNumberRLHidden());
    // deleted
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(2), rFootnotes[3]->GetFootnote().GetNumber());
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(1), rFootnotes[3]->GetFootnote().GetNumberRLHidden());
    // deleted
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(3), rFootnotes[4]->GetFootnote().GetNumber());
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(1), rFootnotes[4]->GetFootnote().GetNumberRLHidden());
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(4), rFootnotes[5]->GetFootnote().GetNumber());
    CPPUNIT_ASSERT_EQUAL(sal_uInt16(2), rFootnotes[5]->GetFootnote().GetNumberRLHidden());
}

DECLARE_ODFEXPORT_TEST(testFdo43807, "fdo43807.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<beans::XPropertySet> xSet(getParagraph(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Drop Caps"),getProperty<OUString>(xSet,"DropCapCharStyleName"));

    xSet.set(getParagraph(2), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("User Defined Drop Caps"),getProperty<OUString>(xSet,"DropCapCharStyleName"));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf103091)
{
    loadAndReload("tdf103091.fodt");
    // check that all conditional paragraph style conditions are imported
    uno::Reference<container::XNameAccess> xParaStyles(getStyles("ParagraphStyles"));
    uno::Reference<beans::XPropertySet> xStyle1(xParaStyles->getByName(
            "Text body"), uno::UNO_QUERY);
    auto conditions(getProperty<uno::Sequence<beans::NamedValue>>(xStyle1, "ParaStyleConditions"));

    CPPUNIT_ASSERT_EQUAL(sal_Int32(28), conditions.getLength());
    CPPUNIT_ASSERT_EQUAL(OUString("TableHeader"), conditions[0].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Addressee")), conditions[0].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("Table"), conditions[1].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Bibliography 1")), conditions[1].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("Frame"), conditions[2].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Bibliography Heading")), conditions[2].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("Section"), conditions[3].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Caption")), conditions[3].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("Footnote"), conditions[4].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Salutation")), conditions[4].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("Endnote"), conditions[5].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 1")), conditions[5].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("Header"), conditions[6].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 2")), conditions[6].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("Footer"), conditions[7].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 3")), conditions[7].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel1"), conditions[8].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 4")), conditions[8].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel2"), conditions[9].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 5")), conditions[9].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel3"), conditions[10].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 6")), conditions[10].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel4"), conditions[11].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 7")), conditions[11].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel5"), conditions[12].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 8")), conditions[12].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel6"), conditions[13].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 9")), conditions[13].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel7"), conditions[14].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents 10")), conditions[14].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel8"), conditions[15].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Contents Heading")), conditions[15].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel9"), conditions[16].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Standard")), conditions[16].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("OutlineLevel10"), conditions[17].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Drawing")), conditions[17].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel1"), conditions[18].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Endnote")), conditions[18].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel2"), conditions[19].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("First line indent")), conditions[19].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel3"), conditions[20].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Footer")), conditions[20].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel4"), conditions[21].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Footer left")), conditions[21].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel5"), conditions[22].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Footer right")), conditions[22].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel6"), conditions[23].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Footnote")), conditions[23].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel7"), conditions[24].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Frame contents")), conditions[24].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel8"), conditions[25].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Hanging indent")), conditions[25].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel9"), conditions[26].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Header")), conditions[26].Value);
    CPPUNIT_ASSERT_EQUAL(OUString("NumberingLevel10"), conditions[27].Name);
    CPPUNIT_ASSERT_EQUAL(uno::Any(OUString("Header left")), conditions[27].Value);
}

DECLARE_ODFEXPORT_TEST(testTextframeTransparentShadow, "textframe-transparent-shadow.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<drawing::XShape> xPicture = getShape(1);
    // ODF stores opacity of 75%, that means 25% transparency.
    CPPUNIT_ASSERT_EQUAL(sal_Int32(25), getProperty<sal_Int32>(xPicture, "ShadowTransparence"));
}

DECLARE_ODFEXPORT_TEST(testRelhPage, "relh-page.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<drawing::XShape> xTextFrame = getShape(1);
    // This was text::RelOrientation::FRAME (the default), RelativeHeightRelation was not handled in xmloff.
    CPPUNIT_ASSERT_EQUAL(text::RelOrientation::PAGE_FRAME, getProperty<sal_Int16>(xTextFrame, "RelativeHeightRelation"));
    // Make sure rel-height-rel doesn't affect width.
    CPPUNIT_ASSERT_EQUAL(text::RelOrientation::FRAME, getProperty<sal_Int16>(xTextFrame, "RelativeWidthRelation"));

    // This was 2601, 20% height was relative from margin, not page.
    CPPUNIT_ASSERT_EQUAL(sal_Int32(3168), parseDump("/root/page/body/txt/anchored/fly/infos/bounds", "height").toInt32());
}

DECLARE_ODFEXPORT_TEST(testRelhPageTdf80282, "relh-page-tdf80282.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<drawing::XShape> xTextFrame = getShape(1);
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Height", sal_Int32(8391), parseDump("//anchored/fly/infos/bounds", "height").toInt32());
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Width",  sal_Int32(5953), parseDump("//anchored/fly/infos/bounds", "width").toInt32());
}

DECLARE_ODFEXPORT_TEST(testRelwPage, "relw-page.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<drawing::XShape> xTextFrame = getShape(1);
    // This was text::RelOrientation::FRAME (the default), RelativeWidthRelation was not handled in xmloff.
    CPPUNIT_ASSERT_EQUAL(text::RelOrientation::PAGE_FRAME, getProperty<sal_Int16>(xTextFrame, "RelativeWidthRelation"));
    // Make sure rel-width-rel doesn't affect height.
    CPPUNIT_ASSERT_EQUAL(text::RelOrientation::FRAME, getProperty<sal_Int16>(xTextFrame, "RelativeHeightRelation"));

    // This was 3762, 40% width was relative from margin, not page.
    CPPUNIT_ASSERT_EQUAL(sal_Int32(4896), parseDump("/root/page/body/txt/anchored/fly/infos/bounds", "width").toInt32());
}

DECLARE_ODFEXPORT_TEST(testTextFrameVertAdjust, "textframe-vertadjust.odt")
{
    CPPUNIT_ASSERT_EQUAL(3, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Test import/export of new frame attribute called TextVerticalAdjust

    // 1st frame's context is adjusted to the top
    uno::Reference<beans::XPropertySet> xFrame(getTextFrameByName("Rectangle 1"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(drawing::TextVerticalAdjust_TOP, getProperty<drawing::TextVerticalAdjust>(xFrame, "TextVerticalAdjust"));
    // 2nd frame's context is adjusted to the center
    xFrame.set(getTextFrameByName("Rectangle 2"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(drawing::TextVerticalAdjust_CENTER, getProperty<drawing::TextVerticalAdjust>(xFrame, "TextVerticalAdjust"));
    // 3rd frame's context is adjusted to the bottom
    xFrame.set(getTextFrameByName("Rectangle 3"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(drawing::TextVerticalAdjust_BOTTOM, getProperty<drawing::TextVerticalAdjust>(xFrame, "TextVerticalAdjust"));
}

DECLARE_ODFEXPORT_TEST(testTdf111891_frameVertStyle, "tdf111891_frameVertStyle.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<beans::XPropertySet> xFrame(getShape(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(drawing::TextVerticalAdjust_BOTTOM, getProperty<drawing::TextVerticalAdjust>(xFrame, "TextVerticalAdjust"));
}

DECLARE_ODFEXPORT_TEST(testShapeRelsize, "shape-relsize.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<drawing::XShape> xShape = getShape(1);
    // These were all 0, as style:rel-width/height was ignored on import for shapes.
    CPPUNIT_ASSERT_EQUAL(sal_Int16(40), getProperty<sal_Int16>(xShape, "RelativeWidth"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(20), getProperty<sal_Int16>(xShape, "RelativeHeight"));

    // Relation was "page" for both width and height, should be "paragraph" for width.
    CPPUNIT_ASSERT_EQUAL(text::RelOrientation::FRAME, getProperty<sal_Int16>(xShape, "RelativeWidthRelation"));
    // And make sure that height stays "page".
    CPPUNIT_ASSERT_EQUAL(text::RelOrientation::PAGE_FRAME, getProperty<sal_Int16>(xShape, "RelativeHeightRelation"));
}

DECLARE_ODFEXPORT_TEST(testTextboxRoundedCorners, "textbox-rounded-corners.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<drawing::XShape> xShape = getShape(1);
    comphelper::SequenceAsHashMap aCustomShapeGeometry(getProperty< uno::Sequence<beans::PropertyValue> >(xShape, "CustomShapeGeometry"));

    // Test that the shape is a rounded rectangle.
    CPPUNIT_ASSERT_EQUAL(OUString("round-rectangle"), aCustomShapeGeometry["Type"].get<OUString>());

    // The shape text should start with a table, with "a" in its A1 cell.
    uno::Reference<text::XText> xText = uno::Reference<text::XTextRange>(xShape, uno::UNO_QUERY_THROW)->getText();
    uno::Reference<text::XTextTable> xTable(getParagraphOrTable(1, xText), uno::UNO_QUERY);
    uno::Reference<text::XTextRange> xCell(xTable->getCellByName("A1"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("a"), xCell->getString());

    // Table inside a textbox should be in the extension namespace.
    if (isExported())
    {
        xmlDocUniquePtr pXmlDoc = parseExport("content.xml");
        // This failed, as draw:custom-shape had a table:table child.
        assertXPath(pXmlDoc, "//draw:custom-shape/loext:table", "name", "Table1");
    }
}

// test that import whitespace collapsing is compatible with old docs
DECLARE_ODFEXPORT_TEST(testWhitespace, "whitespace.odt")
{
    CPPUNIT_ASSERT_EQUAL(4, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<container::XEnumerationAccess> xPara;
    uno::Reference<container::XEnumeration> xPortions;
    uno::Reference<text::XTextRange> xPortion;
    xPara.set(getParagraphOrTable(1), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(2), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    CPPUNIT_ASSERT_EQUAL(OUString("http://example.com/"), getProperty<OUString>(xPortion, "HyperLinkURL"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(3), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Ruby"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(), xPortion->getString());
    CPPUNIT_ASSERT_EQUAL(OUString("foo"), getProperty<OUString>(xPortion, "RubyText"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Ruby"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(4), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("InContentMetadata"), getProperty<OUString>(xPortion, "TextPortionType"));
    {
        // what a stupid idea to require recursively enumerating this
        uno::Reference<container::XEnumerationAccess> xMeta(
            getProperty<uno::Reference<text::XTextContent>>(xPortion, "InContentMetadata"), uno::UNO_QUERY);
        uno::Reference<container::XEnumeration> xMetaPortions =
            xMeta->createEnumeration();
        uno::Reference<text::XTextRange> xMP(xMetaPortions->nextElement(), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xMP, "TextPortionType"));
        CPPUNIT_ASSERT_EQUAL(OUString(" "), xMP->getString());
        CPPUNIT_ASSERT(!xMetaPortions->hasMoreElements());
    }
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(5), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("TextField"), getProperty<OUString>(xPortion, "TextPortionType"));
    {
        // what a stupid idea to require recursively enumerating this
        uno::Reference<container::XEnumerationAccess> xMeta(
            getProperty<uno::Reference<text::XTextContent>>(xPortion, "TextField"), uno::UNO_QUERY);
        uno::Reference<container::XEnumeration> xMetaPortions =
            xMeta->createEnumeration();
        uno::Reference<text::XTextRange> xMP(xMetaPortions->nextElement(), uno::UNO_QUERY);
        CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xMP, "TextPortionType"));
        CPPUNIT_ASSERT_EQUAL(OUString(" "), xMP->getString());
        CPPUNIT_ASSERT(!xMetaPortions->hasMoreElements());
    }
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(7), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Frame"), getProperty<OUString>(xPortion, "TextPortionType"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(8), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Frame"), getProperty<OUString>(xPortion, "TextPortionType"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(9), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Frame"), getProperty<OUString>(xPortion, "TextPortionType"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(10), uno::UNO_QUERY);
    uno::Reference<container::XContentEnumerationAccess> xCEA(xPara, uno::UNO_QUERY);
    uno::Reference<container::XEnumeration> xFrames(
            xCEA->createContentEnumeration("com.sun.star.text.TextContent"));
    xFrames->nextElement(); // one at-paragraph frame
    CPPUNIT_ASSERT(!xFrames->hasMoreElements());
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(11), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Footnote"), getProperty<OUString>(xPortion, "TextPortionType"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(12), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("TextField"), getProperty<OUString>(xPortion, "TextPortionType"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(13), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Annotation"), getProperty<OUString>(xPortion, "TextPortionType"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("AnnotationEnd"), getProperty<OUString>(xPortion, "TextPortionType"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(15), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Bookmark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(16), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Bookmark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Bookmark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(17), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Redline"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Redline"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(18), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Redline"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Redline"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(19), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("ReferenceMark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(20), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("ReferenceMark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("ReferenceMark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(21), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("DocumentIndexMark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());

    xPara.set(getParagraphOrTable(22), uno::UNO_QUERY);
    xPortions.set(xPara->createEnumeration());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString("X "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("DocumentIndexMark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" "), xPortion->getString());
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("DocumentIndexMark"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT(!getProperty<bool>(xPortion, "IsCollapsed"));
    xPortion.set(xPortions->nextElement(), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(OUString("Text"), getProperty<OUString>(xPortion, "TextPortionType"));
    CPPUNIT_ASSERT_EQUAL(OUString(" X"), xPortion->getString());
    CPPUNIT_ASSERT(!xPortions->hasMoreElements());
}

DECLARE_ODFEXPORT_TEST(testTdf136645, "tdf136645.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());

    // Without the fix in place, this would have failed with
    //- Expected: 2640
    //- Actual  : 3000
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2640), parseDump("/root/page/body/section/column[1]/body/infos/bounds", "width").toInt32());

    //- Expected: 6000
    //- Actual  : 6360
    CPPUNIT_ASSERT_EQUAL(sal_Int32(6000), parseDump("/root/page/body/section/column[2]/body/infos/bounds", "width").toInt32());
}

DECLARE_ODFEXPORT_TEST(testBtlrCell, "btlr-cell.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Without the accompanying fix in place, this test would have failed, as
    // the btlr text direction in the A1 cell was lost on ODF import and
    // export.
    uno::Reference<text::XTextTablesSupplier> xSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xTables = xSupplier->getTextTables();
    uno::Reference<text::XTextTable> xTable(xTables->getByName("Table1"), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xA1(xTable->getCellByName("A1"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(text::WritingMode2::BT_LR, getProperty<sal_Int16>(xA1, "WritingMode"));

    uno::Reference<beans::XPropertySet> xB1(xTable->getCellByName("B1"), uno::UNO_QUERY);
    auto nActual = getProperty<sal_Int16>(xB1, "WritingMode");
    CPPUNIT_ASSERT(nActual == text::WritingMode2::LR_TB || nActual == text::WritingMode2::CONTEXT);

    uno::Reference<beans::XPropertySet> xC1(xTable->getCellByName("C1"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(text::WritingMode2::TB_RL, getProperty<sal_Int16>(xC1, "WritingMode"));
}

DECLARE_ODFEXPORT_TEST(testBtlrFrame, "btlr-frame.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Without the accompanying fix in place, this test would have failed, as
    // the btlr text direction in the text frame was lost on ODF import and
    // export.
    uno::Reference<beans::XPropertySet> xTextFrame(getShape(1), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xTextFrame.is());

    auto nActual = getProperty<sal_Int16>(xTextFrame, "WritingMode");
    CPPUNIT_ASSERT_EQUAL(text::WritingMode2::BT_LR, nActual);

    // Without the accompanying fix in place, this test would have failed, as the fly frame had
    // mbVertical==true, but mbVertLRBT==false, even if the writing direction in the doc model was
    // btlr.
    SwXTextDocument* pTextDoc = dynamic_cast<SwXTextDocument *>(mxComponent.get());
    CPPUNIT_ASSERT(pTextDoc);

    SwDoc* pDoc = pTextDoc->GetDocShell()->GetDoc();
    CPPUNIT_ASSERT(pDoc);

    SwRootFrame* pLayout = pDoc->getIDocumentLayoutAccess().GetCurrentLayout();
    CPPUNIT_ASSERT(pLayout);

    SwFrame* pPageFrame = pLayout->GetLower();
    CPPUNIT_ASSERT(pPageFrame);
    CPPUNIT_ASSERT(pPageFrame->IsPageFrame());

    SwFrame* pBodyFrame = pPageFrame->GetLower();
    CPPUNIT_ASSERT(pBodyFrame);
    CPPUNIT_ASSERT(pBodyFrame->IsBodyFrame());

    SwFrame* pBodyTextFrame = pBodyFrame->GetLower();
    CPPUNIT_ASSERT(pBodyTextFrame);
    CPPUNIT_ASSERT(pBodyTextFrame->IsTextFrame());

    CPPUNIT_ASSERT(pBodyTextFrame->GetDrawObjs());
    const SwSortedObjs& rAnchored = *pBodyTextFrame->GetDrawObjs();
    CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(1), rAnchored.size());

    auto* pFlyFrame = dynamic_cast<SwFlyFrame*>(rAnchored[0]);
    CPPUNIT_ASSERT(pFlyFrame);
    CPPUNIT_ASSERT(pFlyFrame->IsVertLRBT());

    if (!isExported())
        // Not yet exported, don't modify the doc model for test purposes.
        return;

    // Make sure that btlr -> tbrl transition clears the "BT" flag.
    xTextFrame->setPropertyValue("WritingMode", uno::Any(text::WritingMode2::TB_LR));
    pFlyFrame = dynamic_cast<SwFlyFrame*>(rAnchored[0]);
    CPPUNIT_ASSERT(pFlyFrame);
    CPPUNIT_ASSERT(!pFlyFrame->IsVertLRBT());
}

CPPUNIT_TEST_FIXTURE(Test, testTdf129520)
{
    loadAndReload("tdf129520.docx");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    CPPUNIT_ASSERT_EQUAL(OUString("M"), getParagraph(1)->getString());

    // Without this fix in place, this test would have failed with
    // - Expected: Ma
    // - Actual  :
    CPPUNIT_ASSERT_EQUAL(OUString("Ma"), getParagraph(2)->getString());
    CPPUNIT_ASSERT_EQUAL(OUString("1815"), getParagraph(3)->getString());
}

DECLARE_ODFEXPORT_TEST(testFdo86963, "fdo86963.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Export of this document failed with beans::UnknownPropertyException.
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
}

// Check for correct header/footer with special first page with TOC inside:
// - DECLARE_ODFEXPORT_TEST(testTdf118393, "tdf118393.odt")
// - DECLARE_OOXMLEXPORT_TEST(testTdf118393, "tdf118393.odt")
DECLARE_ODFEXPORT_TEST(testTdf118393, "tdf118393.odt")
{
    CPPUNIT_ASSERT_EQUAL( 7, getPages() );

    // First page has no header/footer
    {
        xmlDocUniquePtr pXmlDoc = parseLayoutDump();

        // check first page
        assertXPath(pXmlDoc, "/root/page[1]/header", 0);
        assertXPath(pXmlDoc, "/root/page[1]/footer", 0);

        // check second page in the same way
        assertXPath(pXmlDoc, "/root/page[2]/header");
        assertXPath(pXmlDoc, "/root/page[2]/footer");
   }

    // All other pages should have header/footer

    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[2]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[2]/footer/txt/text()"));

    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[3]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[3]/footer/txt/text()"));

    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[4]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[4]/footer/txt/text()"));

    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[5]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[5]/footer/txt/text()"));

    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[6]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[6]/footer/txt/text()"));

    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[7]/header/txt/text()"));
    CPPUNIT_ASSERT_EQUAL(OUString("Seite * von *"),   parseDump("/root/page[7]/footer/txt/text()"));
}

DECLARE_ODFEXPORT_TEST(testTdf135338_firstLeftPageFooter, "tdf135338_firstLeftPageFooter.odt")
{
    CPPUNIT_ASSERT_EQUAL(6, getPages());
    // The first page is a left page only style, but it should still show the first page footer
    // instead of the left footer text "EVEN/LEFT (Left page only)"
    CPPUNIT_ASSERT_EQUAL(OUString("First (Left page only)"),  parseDump("/root/page[2]/footer/txt/text()"));
}

DECLARE_ODFEXPORT_TEST(testGerrit13858, "gerrit13858.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Just make sure the output is valid.
}
DECLARE_ODFEXPORT_TEST(testOdtBorderTypes, "border_types.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    static const sal_Int32 lineStyles[] = { 0, 1, 2, 14, 16, 17, 3, 15 };
    uno::Reference<text::XTextDocument> textDocument(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XEnumerationAccess> xParaEnumAccess(textDocument->getText(), uno::UNO_QUERY);
    // list of paragraphs
    uno::Reference<container::XEnumeration> xParaEnum = xParaEnumAccess->createEnumeration();
    do
    {
        uno::Reference<lang::XServiceInfo> xServiceInfo;
        if (xParaEnum->nextElement() >>= xServiceInfo)
        {
            if (xServiceInfo->supportsService("com.sun.star.text.TextTable"))
            {
                uno::Reference<table::XCellRange> const xCellRange(xServiceInfo, uno::UNO_QUERY_THROW);

                for (sal_Int32 row = 0; row < 15; row += 2)
                {
                    uno::Reference<table::XCell> xCell = xCellRange->getCellByPosition(1, row);
                    uno::Reference< beans::XPropertySet > xPropSet(xCell, uno::UNO_QUERY_THROW);

                    uno::Any aTopBorder = xPropSet->getPropertyValue("TopBorder");
                    table::BorderLine2 aTopBorderLine;
                    if (aTopBorder >>= aTopBorderLine)
                    {
                        sal_Int32 lineStyle = aTopBorderLine.LineStyle;
                        CPPUNIT_ASSERT_EQUAL(lineStyles[row / 2], lineStyle);
                    }
                }   //end of the 'for' loop
            }
        }
    } while (xParaEnum->hasMoreElements());
}

DECLARE_ODFEXPORT_TEST(testMasterPageWithDrawingPage, "sw_hatch.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<container::XNameAccess> xStyles(getStyles("PageStyles"));
    uno::Reference<beans::XPropertySet> xStyle(xStyles->getByName("Standard"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(drawing::FillStyle_HATCH, getProperty<drawing::FillStyle>(xStyle, "FillStyle"));
    CPPUNIT_ASSERT_EQUAL(OUString("Blue -45 Degrees"), getProperty<OUString>(xStyle, "FillHatchName"));
    CPPUNIT_ASSERT(!getProperty<sal_Bool>(xStyle, "FillBackground"));
    CPPUNIT_ASSERT_EQUAL(sal_Int16(0), getProperty<sal_Int16>(xStyle, "FillTransparence"));
}

CPPUNIT_TEST_FIXTURE(Test, testPageStyleBackgroundFullSizeOOo)
{
    loadAndReload("pagestyle_background_ooo33.odt");
    CPPUNIT_ASSERT_EQUAL(3, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");
    // Standard
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "border");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "solid");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-color", "#99ccff");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "opacity", "100%");
    // Endnote
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "border");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "repeat", "repeat");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-image-ref-point", "top-left");
    // Footnote
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "border");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "repeat", "stretch");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-image-ref-point", "top-left");
}

CPPUNIT_TEST_FIXTURE(Test, testPageStyleBackgroundFullSizeLO64)
{
    loadAndReload("pagestyle_background_lo64.odt");
    CPPUNIT_ASSERT_EQUAL(6, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");
    // Standard
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "full");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "solid");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-color", "#99ccff");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "opacity", "100%");
    // Endnote
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "full");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "repeat", "repeat");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-image-ref-point", "top-left");
    // Footnote
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "border");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "repeat", "stretch");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-image-ref-point", "top-left");
    // Landscape
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Landscape']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "border");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Landscape']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Landscape']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "repeat", "no-repeat");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Landscape']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-image-ref-point", "top-left");
    // Index
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Index']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "full");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Index']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "gradient");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Index']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "gradient-step-count", "0");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Index']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "opacity", "100%");
    // First Page
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='First_20_Page']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "full");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='First_20_Page']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "hatch");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='First_20_Page']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-hatch-solid", "false");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='First_20_Page']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "opacity", "100%");
}

CPPUNIT_TEST_FIXTURE(Test, testPageStyleBackgroundFullSizeLO70)
{
    loadAndReload("pagestyle_background_lo70.odt");
    CPPUNIT_ASSERT_EQUAL(6, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");
    // Standard
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "full");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "solid");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-color", "#99ccff");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Standard']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "opacity", "100%");
    // Endnote
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "full");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "repeat", "repeat");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Endnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-image-ref-point", "top-left");
    // Footnote
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "border");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "repeat", "stretch");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Footnote']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-image-ref-point", "top-left");
    // Landscape
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Landscape']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "border");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Landscape']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Landscape']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "repeat", "no-repeat");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Landscape']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-image-ref-point", "top-left");
    // Index
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Index']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "full");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Index']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "gradient");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Index']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "gradient-step-count", "0");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='Index']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "opacity", "100%");
    // First Page
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='First_20_Page']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "background-size", "full");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='First_20_Page']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill", "hatch");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='First_20_Page']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "fill-hatch-solid", "false");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:family='drawing-page' and @style:name = "
        "/office:document-styles/office:master-styles/style:master-page[@style:name='First_20_Page']/attribute::draw:style-name"
        "]/style:drawing-page-properties", "opacity", "100%");
}

CPPUNIT_TEST_FIXTURE(Test, testFillBitmapUnused)
{
    loadAndReload("fillbitmap3.odt");
    CPPUNIT_ASSERT_EQUAL(4, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // nav_5f_home and all its references are completely gone
    xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");

    // paragraph style
    assertXPath(pXmlDoc,
        "/office:document-styles/office:styles/style:style[@style:name='Text_20_body']/loext:graphic-properties", "fill", "solid");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:styles/style:style[@style:name='Text_20_body']/loext:graphic-properties", "fill-color", "#c0c0c0");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:styles/style:style[@style:name='Text_20_body']/loext:graphic-properties[@draw:fill-image-name]", 0);

    // page style page-layout
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm1']/style:page-layout-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm1']/style:page-layout-properties", "fill-image-name", "nav_5f_up");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm1']/style:header-style/style:header-footer-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm1']/style:header-style/style:header-footer-properties", "fill-image-name", "nav_5f_up");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm1']/style:footer-style/style:header-footer-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm1']/style:footer-style/style:header-footer-properties", "fill-image-name", "nav_5f_up");

    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm2']/style:page-layout-properties", "fill", "solid");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm2']/style:page-layout-properties[@draw:fill-image-name]", 0);
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm2']/style:header-style/style:header-footer-properties", "fill", "solid");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm2']/style:header-style/style:header-footer-properties[@draw:fill-image-name]", 0);
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm2']/style:footer-style/style:header-footer-properties", "fill", "solid");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:page-layout[@style:name='Mpm2']/style:footer-style/style:header-footer-properties[@draw:fill-image-name]", 0);

    // page style drawing-page
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:name='Mdp1']/style:drawing-page-properties", "fill", "bitmap");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:name='Mdp1']/style:drawing-page-properties", "fill-image-name", "nav_5f_up");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:name='Mdp2']/style:drawing-page-properties", "fill", "solid");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:name='Mdp2']/style:drawing-page-properties", "fill-color", "#c0c0c0");
    assertXPath(pXmlDoc,
        "/office:document-styles/office:automatic-styles/style:style[@style:name='Mdp2']/style:drawing-page-properties[@draw:fill-image-name]", 0);

    // the named items
    assertXPath(pXmlDoc,
        "/office:document-styles/office:styles/draw:fill-image", 1);
    assertXPath(pXmlDoc,
        "/office:document-styles/office:styles/draw:fill-image", "name", "nav_5f_up");
}

DECLARE_ODFEXPORT_TEST(testCellUserDefineAttr, "userdefattr-tablecell.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<text::XTextTable> xTable(getParagraphOrTable(1), uno::UNO_QUERY);
    uno::Reference<table::XCell> const xCellA1(xTable->getCellByName("A1"), uno::UNO_SET_THROW);
    uno::Reference<table::XCell> const xCellB1(xTable->getCellByName("B1"), uno::UNO_SET_THROW);
    uno::Reference<table::XCell> const xCellC1(xTable->getCellByName("C1"), uno::UNO_SET_THROW);
    getUserDefineAttribute(uno::Any(xCellA1), "proName", "v1");
    getUserDefineAttribute(uno::Any(xCellB1), "proName", "v2");
    getUserDefineAttribute(uno::Any(xCellC1), "proName", "v3");
}

DECLARE_ODFEXPORT_TEST(testEmbeddedPdf, "embedded-pdf.odt")
{
    auto pPdfium = vcl::pdf::PDFiumLibrary::get();
    if (!pPdfium)
    {
        return;
    }

    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    uno::Reference<drawing::XShape> xShape = getShape(1);
    // This failed, pdf+png replacement graphics pair didn't survive an ODT roundtrip.
    auto xReplacementGraphic = getProperty<uno::Reference<graphic::XGraphic>>(xShape, "ReplacementGraphic");
    CPPUNIT_ASSERT(xReplacementGraphic.is());

    auto xGraphic = getProperty<uno::Reference<graphic::XGraphic>>(xShape, "Graphic");
    CPPUNIT_ASSERT(xGraphic.is());
    // This was image/x-vclgraphic, not exposing the info that the image is a PDF one.
    CPPUNIT_ASSERT_EQUAL(OUString("application/pdf"), getProperty<OUString>(xGraphic, "MimeType"));

    if (isExported())
    {
        uno::Sequence<uno::Any> aArgs{ uno::Any(maTempFile.GetURL()) };
        uno::Reference<container::XNameAccess> xNameAccess(m_xSFactory->createInstanceWithArguments("com.sun.star.packages.zip.ZipFileAccess", aArgs), uno::UNO_QUERY);
        bool bHasBitmap = false;
        const uno::Sequence<OUString> aNames = xNameAccess->getElementNames();
        for (const auto& rElementName : aNames)
        {
            if (rElementName.startsWith("Pictures") && rElementName.endsWith("png"))
            {
                bHasBitmap = true;
                break;
            }
        }
        // This failed, replacement was an svm file.
        CPPUNIT_ASSERT(bHasBitmap);
    }
}

DECLARE_ODFEXPORT_TEST(testTableStyles1, "table_styles_1.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Table styles basic graphic test.
    // Doesn't cover all attributes.
    uno::Reference<style::XStyleFamiliesSupplier> XFamiliesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xFamilies(XFamiliesSupplier->getStyleFamilies());
    uno::Reference<container::XNameAccess> xCellFamily(xFamilies->getByName("CellStyles"), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xCell1Style;
    xCellFamily->getByName("Test style.1") >>= xCell1Style;

    sal_Int32 nInt32 = 0xF0F0F0;
    table::BorderLine2 oBorder;

    CPPUNIT_ASSERT_EQUAL(Color(0xCC0000), getProperty<Color>(xCell1Style, "BackColor"));
    xCell1Style->getPropertyValue("WritingMode") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(4), nInt32);
    xCell1Style->getPropertyValue("VertOrient") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nInt32);
    xCell1Style->getPropertyValue("BorderDistance") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(97), nInt32);
    xCell1Style->getPropertyValue("LeftBorderDistance") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(97), nInt32);
    xCell1Style->getPropertyValue("RightBorderDistance") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(97), nInt32);
    xCell1Style->getPropertyValue("TopBorderDistance") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(97), nInt32);
    xCell1Style->getPropertyValue("BottomBorderDistance") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(97), nInt32);
    xCell1Style->getPropertyValue("RightBorder") >>= oBorder;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), oBorder.Color);
    xCell1Style->getPropertyValue("LeftBorder") >>= oBorder;
    CPPUNIT_ASSERT_EQUAL(COL_BLACK, Color(ColorTransparency, oBorder.Color));
    xCell1Style->getPropertyValue("TopBorder") >>= oBorder;
    CPPUNIT_ASSERT_EQUAL(COL_BLACK, Color(ColorTransparency, oBorder.Color));
    xCell1Style->getPropertyValue("BottomBorder") >>= oBorder;
    CPPUNIT_ASSERT_EQUAL(COL_BLACK, Color(ColorTransparency, oBorder.Color));
}

DECLARE_ODFEXPORT_TEST(testTableStyles2, "table_styles_2.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Table styles paragraph and char tests
    // Doesn't cover all attributes.
    // Problem: underline for table autoformat doesn't work.
    uno::Reference<style::XStyleFamiliesSupplier> XFamiliesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xFamilies(XFamiliesSupplier->getStyleFamilies());
    uno::Reference<container::XNameAccess> xTableFamily(xFamilies->getByName("TableStyles"), uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xTableStyle(xTableFamily->getByName("Test style2"), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xCell1Style;

    float fFloat = 0.;
    bool bBool = true;
    sal_Int16 nInt16 = 0xF0;
    sal_Int32 nInt32 = 0xF0F0F0;
    OUString sString;
    awt::FontSlant eCharPosture;

    // cell 1
    xTableStyle->getByName("first-row-start-column") >>= xCell1Style;
    xCell1Style->getPropertyValue("ParaAdjust") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nInt32);
    CPPUNIT_ASSERT_EQUAL(Color(0xFF6600), getProperty<Color>(xCell1Style, "CharColor"));
    xCell1Style->getPropertyValue("CharContoured") >>= bBool;
    CPPUNIT_ASSERT_EQUAL(false, bBool);
    xCell1Style->getPropertyValue("CharShadowed") >>= bBool;
    CPPUNIT_ASSERT_EQUAL(true, bBool);
    xCell1Style->getPropertyValue("CharStrikeout") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), nInt32);
    xCell1Style->getPropertyValue("CharUnderline") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nInt32);
    // underline color is not working for table autoformats
    // xCell1Style->getPropertyValue("CharUnderlineHasColor") >>= bBool;
    // CPPUNIT_ASSERT_EQUAL(bool(false), bBool);
    // xCell1Style->getPropertyValue("CharUnderlineColor") >>= nInt64;
    // CPPUNIT_ASSERT_EQUAL(sal_Int64(-1), nInt64);
    // standard font
    xCell1Style->getPropertyValue("CharHeight") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(18.), fFloat);
    xCell1Style->getPropertyValue("CharWeight") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(100.), fFloat);
    xCell1Style->getPropertyValue("CharPosture") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_NONE, eCharPosture);
    xCell1Style->getPropertyValue("CharFontName") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Courier"), sString);
    xCell1Style->getPropertyValue("CharFontStyleName") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString(), sString);
    xCell1Style->getPropertyValue("CharFontFamily") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);
    xCell1Style->getPropertyValue("CharFontPitch") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), nInt16);
    // cjk font
    xCell1Style->getPropertyValue("CharHeightAsian") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(18.), fFloat);
    xCell1Style->getPropertyValue("CharWeightAsian") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(100.), fFloat);
    xCell1Style->getPropertyValue("CharPostureAsian") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_NONE, eCharPosture);
    xCell1Style->getPropertyValue("CharFontNameAsian") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Courier"), sString);
    xCell1Style->getPropertyValue("CharFontStyleNameAsian") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Regularna"), sString);
    xCell1Style->getPropertyValue("CharFontFamilyAsian") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);
    xCell1Style->getPropertyValue("CharFontPitchAsian") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), nInt16);
    // ctl font
    xCell1Style->getPropertyValue("CharHeightComplex") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(18.), fFloat);
    xCell1Style->getPropertyValue("CharWeightComplex") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(100.), fFloat);
    xCell1Style->getPropertyValue("CharPostureComplex") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_NONE, eCharPosture);
    xCell1Style->getPropertyValue("CharFontNameComplex") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Courier"), sString);
    xCell1Style->getPropertyValue("CharFontStyleNameComplex") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Regularna"), sString);
    xCell1Style->getPropertyValue("CharFontFamilyComplex") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);
    xCell1Style->getPropertyValue("CharFontPitchComplex") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(1), nInt16);

    // cell 2
    xTableStyle->getByName("first-row") >>= xCell1Style;
    xCell1Style->getPropertyValue("ParaAdjust") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(3), nInt32);
    CPPUNIT_ASSERT_EQUAL(Color(0x9900FF), getProperty<Color>(xCell1Style, "CharColor"));
    xCell1Style->getPropertyValue("CharContoured") >>= bBool;
    CPPUNIT_ASSERT_EQUAL(true, bBool);
    xCell1Style->getPropertyValue("CharShadowed") >>= bBool;
    CPPUNIT_ASSERT_EQUAL(false, bBool);
    xCell1Style->getPropertyValue("CharStrikeout") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nInt32);
    xCell1Style->getPropertyValue("CharUnderline") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(2), nInt32);
    // underline color test place
    // standard font
    xCell1Style->getPropertyValue("CharHeight") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(12.), fFloat);
    xCell1Style->getPropertyValue("CharWeight") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(150.), fFloat);
    xCell1Style->getPropertyValue("CharPosture") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_NONE, eCharPosture);
    xCell1Style->getPropertyValue("CharFontName") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Liberation Serif"), sString);
    xCell1Style->getPropertyValue("CharFontStyleName") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString(), sString);
    xCell1Style->getPropertyValue("CharFontFamily") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(3), nInt16);
    xCell1Style->getPropertyValue("CharFontPitch") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);
    // cjk font
    xCell1Style->getPropertyValue("CharHeightAsian") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(12.), fFloat);
    xCell1Style->getPropertyValue("CharWeightAsian") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(150.), fFloat);
    xCell1Style->getPropertyValue("CharPostureAsian") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_NONE, eCharPosture);
    xCell1Style->getPropertyValue("CharFontNameAsian") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Liberation Serif"), sString);
    xCell1Style->getPropertyValue("CharFontStyleNameAsian") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Pogrubiona"), sString);
    xCell1Style->getPropertyValue("CharFontFamilyAsian") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(3), nInt16);
    xCell1Style->getPropertyValue("CharFontPitchAsian") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);
    // ctl font
    xCell1Style->getPropertyValue("CharHeightComplex") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(12.), fFloat);
    xCell1Style->getPropertyValue("CharWeightComplex") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(150.), fFloat);
    xCell1Style->getPropertyValue("CharPostureComplex") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_NONE, eCharPosture);
    xCell1Style->getPropertyValue("CharFontNameComplex") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Liberation Serif"), sString);
    xCell1Style->getPropertyValue("CharFontStyleNameComplex") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Pogrubiona"), sString);
    xCell1Style->getPropertyValue("CharFontFamilyComplex") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(3), nInt16);
    xCell1Style->getPropertyValue("CharFontPitchComplex") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);

    // cell 3
    xTableStyle->getByName("first-row-even-column") >>= xCell1Style;
    xCell1Style->getPropertyValue("ParaAdjust") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(1), nInt32);
    CPPUNIT_ASSERT_EQUAL(COL_BLACK, getProperty<Color>(xCell1Style, "CharColor"));
    xCell1Style->getPropertyValue("CharContoured") >>= bBool;
    CPPUNIT_ASSERT_EQUAL(true, bBool);
    xCell1Style->getPropertyValue("CharShadowed") >>= bBool;
    CPPUNIT_ASSERT_EQUAL(true, bBool);
    xCell1Style->getPropertyValue("CharStrikeout") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(0), nInt32);
    xCell1Style->getPropertyValue("CharUnderline") >>= nInt32;
    CPPUNIT_ASSERT_EQUAL(sal_Int32(6), nInt32);
    // underline color test place
    // standard font
    xCell1Style->getPropertyValue("CharHeight") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(12.), fFloat);
    xCell1Style->getPropertyValue("CharWeight") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(100.), fFloat);
    xCell1Style->getPropertyValue("CharPosture") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_ITALIC, eCharPosture);
    xCell1Style->getPropertyValue("CharFontName") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Open Sans"), sString);
    xCell1Style->getPropertyValue("CharFontStyleName") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString(), sString);
    xCell1Style->getPropertyValue("CharFontFamily") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(0), nInt16);
    xCell1Style->getPropertyValue("CharFontPitch") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);
    // cjk font
    xCell1Style->getPropertyValue("CharHeightAsian") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(12.), fFloat);
    xCell1Style->getPropertyValue("CharWeightAsian") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(100.), fFloat);
    xCell1Style->getPropertyValue("CharPostureAsian") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_ITALIC, eCharPosture);
    xCell1Style->getPropertyValue("CharFontNameAsian") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Open Sans"), sString);
    xCell1Style->getPropertyValue("CharFontStyleNameAsian") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Kursywa"), sString);
    xCell1Style->getPropertyValue("CharFontFamilyAsian") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(0), nInt16);
    xCell1Style->getPropertyValue("CharFontPitchAsian") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);
    // ctl font
    xCell1Style->getPropertyValue("CharHeightComplex") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(12.), fFloat);
    xCell1Style->getPropertyValue("CharWeightComplex") >>= fFloat;
    CPPUNIT_ASSERT_EQUAL(float(100.), fFloat);
    xCell1Style->getPropertyValue("CharPostureComplex") >>= eCharPosture;
    CPPUNIT_ASSERT_EQUAL(awt::FontSlant_ITALIC, eCharPosture);
    xCell1Style->getPropertyValue("CharFontNameComplex") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Open Sans"), sString);
    xCell1Style->getPropertyValue("CharFontStyleNameComplex") >>= sString;
    CPPUNIT_ASSERT_EQUAL(OUString("Kursywa"), sString);
    xCell1Style->getPropertyValue("CharFontFamilyComplex") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(0), nInt16);
    xCell1Style->getPropertyValue("CharFontPitchComplex") >>= nInt16;
    CPPUNIT_ASSERT_EQUAL(sal_Int16(2), nInt16);
}

CPPUNIT_TEST_FIXTURE(Test, testTableStyles3)
{
    loadAndReload("table_styles_3.odt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // This test checks if default valued attributes aren't exported.
    xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");

    // <style:paragraph-properties>
    // For this element the only exported attributes are: "border-left", "border-bottom"
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "background-color");
    // border-left place
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "border-right");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "border-top");
    // border-bottom place
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "padding");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "padding-left");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "padding-right");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "padding-top");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "padding-bottom");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:table-cell-properties", "writing-mode");

    // <style:paragraph-properties> should be absent, because it has only "text-align" attribute, which shouldn't be exported.
    // Assume that style:paragraph-properties and style:text-properties exists.
    assertXPathChildren(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']", 2);

    // <style:text-properties>
    // For this element the only exported attributes are: "use-window-font-color place", "font-size-asian", "font-name-asian", "font-family-asian", "font-name-complex", "font-family-complex"
    // use-window-font-color place
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "text-shadow");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "text-outline");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "text-line-through-style");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "text-line-through-type");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "text-underline-style");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "text-underline-color");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-size");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-weight");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-style");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-family");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-family-generic");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-pitch");
    // font-size-asian place
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-weight-asian");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-style-asian");
    // font-name-asian place
    // font-family-asian place
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-style-name-asian");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-family-generic-asian");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-pitch-asian");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-size-complex");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-weight-complex");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-style-complex");
    // font-name-complex place
    // font-family-complex place
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-style-name-complex");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-family-generic-complex");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style3.2']/style:text-properties", "font-pitch-complex");

}

CPPUNIT_TEST_FIXTURE(Test, testTableStyles4)
{
    createSwDoc("table_styles_4.odt");
    // Test if loaded styles overwrite existing styles
    uno::Reference<style::XStyleFamiliesSupplier> XFamiliesSupplier(mxComponent, uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xFamilies(XFamiliesSupplier->getStyleFamilies());
    uno::Reference<container::XNameAccess> xTableFamily(xFamilies->getByName("TableStyles"), uno::UNO_QUERY);
    uno::Reference<container::XNameAccess> xTableStyle(xTableFamily->getByName("Green"), uno::UNO_QUERY);
    uno::Reference<beans::XPropertySet> xCell1Style;

    xTableStyle->getByName("first-row-start-column") >>= xCell1Style;
    CPPUNIT_ASSERT_EQUAL(Color(0x00ff00), getProperty<Color>(xCell1Style, "BackColor"));
}

CPPUNIT_TEST_FIXTURE(Test, testTableStyles5)
{
    loadAndReload("table_styles_5.odt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Test if cell styles doesn't have a style:parent-style-name attribute.
    xmlDocUniquePtr pXmlDoc = parseExport("styles.xml");

    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.1']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.2']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.3']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.4']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.5']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.6']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.7']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.8']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.9']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.10']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.11']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.12']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.13']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.14']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.15']", "parent-style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-styles/office:styles/style:style[@style:display-name='Test style.16']", "parent-style-name");

}

CPPUNIT_TEST_FIXTURE(Test, testTdf145226)
{
    loadAndReload("tdf145226.fodt");
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    xmlDocUniquePtr pXmlDoc = parseExport("content.xml");

    assertXPathNoAttribute(pXmlDoc, "/office:document-content/office:body/office:text/table:table/table:table-row[1]", "style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-content/office:body/office:text/table:table/table:table-row[2]", "style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-content/office:body/office:text/table:table/table:table-row[3]", "style-name");
    assertXPathNoAttribute(pXmlDoc, "/office:document-content/office:body/office:text/table:table/table:table-row[4]", "style-name");
}

DECLARE_ODFEXPORT_TEST(testTdf101710, "tdf101710.odt")
{
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Test that number format of cell styles can be imported and exported.
    uno::Reference<beans::XPropertySet> xStyle(getStyles("CellStyles")->getByName("Test Style.11"), uno::UNO_QUERY);
    CPPUNIT_ASSERT_EQUAL(sal_uInt32(10104), getProperty<sal_uInt32>(xStyle, "NumberFormat"));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf129568)
{
    loadAndReload("tdf129568.fodt");
    // Test that export doesn't fail, and that style is imported and in use.
    uno::Reference<style::XStyle> xStyle(getStyles("CellStyles")->getByName("Default Style.1"), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xStyle->isInUse());
    CPPUNIT_ASSERT_EQUAL(Color(0xffff00), getProperty<Color>(xStyle, "BackColor"));
}

CPPUNIT_TEST_FIXTURE(Test, testTdf129568ui)
{
    loadAndReload("tdf129568-ui.fodt");
    // Same as above, but styles referenced by UI name.
    uno::Reference<style::XStyle> xStyle(getStyles("CellStyles")->getByName("Default Style.1"), uno::UNO_QUERY);
    CPPUNIT_ASSERT(xStyle->isInUse());
    CPPUNIT_ASSERT_EQUAL(Color(0xffff00), getProperty<Color>(xStyle, "BackColor"));
}

DECLARE_ODFEXPORT_TEST(testTdf132642_keepWithNextTable, "tdf132642_keepWithNextTable.odt")
{
    // Since the row is very big, it should split over two pages.
    // Since up to this point we haven't tried to make it match MS formats, it should start on page 1.
    CPPUNIT_ASSERT_EQUAL_MESSAGE("Row splits over 2 pages", 2, getPages());
}

CPPUNIT_TEST_FIXTURE(Test, testImageMimetype)
{
    loadAndReload("image-mimetype.odt");
    CPPUNIT_ASSERT_EQUAL(1, getShapes());
    CPPUNIT_ASSERT_EQUAL(1, getPages());
    // Test that the loext:mimetype attribute is written for exported images, tdf#109202
    xmlDocUniquePtr pXmlDoc = parseExport("content.xml");
    // Original image (svg)
    assertXPath(pXmlDoc, "/office:document-content/office:body/office:text/text:p/draw:frame/draw:image[@draw:mime-type='image/svg+xml']");
}


CPPUNIT_PLUGIN_IMPLEMENT();
/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
