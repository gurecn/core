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


#include <com/sun/star/linguistic2/XThesaurus.hpp>

#include <comphelper/string.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/namedcolor.hxx>
#include <sfx2/request.hxx>
#include <svx/svdview.hxx>
#include <editeng/spltitem.hxx>
#include <editeng/lrspitem.hxx>
#include <editeng/ulspitem.hxx>
#include <editeng/orphitem.hxx>
#include <editeng/formatbreakitem.hxx>
#include <editeng/widwitem.hxx>
#include <editeng/kernitem.hxx>
#include <editeng/escapementitem.hxx>
#include <editeng/lspcitem.hxx>
#include <editeng/flstitem.hxx>
#include <editeng/adjustitem.hxx>
#include <editeng/udlnitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/colritem.hxx>
#include <editeng/frmdiritem.hxx>
#include <editeng/urlfieldhelper.hxx>
#include <svx/svdoutl.hxx>
#include <sfx2/viewfrm.hxx>
#include <svl/stritem.hxx>
#include <svl/whiter.hxx>
#include <svl/cjkoptions.hxx>
#include <svl/ctloptions.hxx>
#include <svl/languageoptions.hxx>
#include <editeng/flditem.hxx>
#include <editeng/editstat.hxx>
#include <svx/clipfmtitem.hxx>
#include <svx/hlnkitem.hxx>
#include <svx/svxdlg.hxx>
#include <sfx2/htmlmode.hxx>
#include <editeng/langitem.hxx>
#include <editeng/scriptsetitem.hxx>
#include <editeng/writingmodeitem.hxx>
#include <editeng/eeitem.hxx>
#include <editeng/editdata.hxx>
#include <editeng/outliner.hxx>
#include <editeng/editview.hxx>
#include <vcl/unohelp2.hxx>
#include <editeng/hyphenzoneitem.hxx>
#include <osl/diagnose.h>

#include <cmdid.h>
#include <doc.hxx>
#include <drwtxtsh.hxx>
#include <edtwin.hxx>
#include <hintids.hxx>
#include <langhelper.hxx>
#include <chrdlgmodes.hxx>
#include <swmodule.hxx>
#include <uitool.hxx>
#include <viewopt.hxx>
#include <wrtsh.hxx>
#include <wview.hxx>

#include <swabstdlg.hxx>

using namespace ::com::sun::star;

void SwDrawTextShell::Execute( SfxRequest &rReq )
{
    SwWrtShell &rSh = GetShell();
    OutlinerView* pOLV = m_pSdrView->GetTextEditOutlinerView();
    SfxItemSet aEditAttr(pOLV->GetAttribs());
    SfxItemSet aNewAttr(*aEditAttr.GetPool(), aEditAttr.GetRanges());

    const sal_uInt16 nSlot = rReq.GetSlot();

    const sal_uInt16 nWhich = GetPool().GetWhichIDFromSlotID(nSlot);
    std::unique_ptr<SfxItemSet> pNewAttrs(rReq.GetArgs() ? rReq.GetArgs()->Clone() : nullptr);

    bool bRestoreSelection = false;
    ESelection aOldSelection;

    sal_uInt16 nEEWhich = 0;
    switch (nSlot)
    {
        case SID_LANGUAGE_STATUS:
        {
            aOldSelection = pOLV->GetSelection();
            if (!pOLV->GetEditView().HasSelection())
            {
                pOLV->GetEditView().SelectCurrentWord();
            }

            bRestoreSelection = SwLangHelper::SetLanguageStatus(pOLV,rReq,GetView(),rSh);
            break;
        }

        case SID_THES:
        {
            OUString aReplaceText;
            const SfxStringItem* pItem2 = rReq.GetArg(FN_PARAM_THES_WORD_REPLACE);
            if (pItem2)
                aReplaceText = pItem2->GetValue();
            if (!aReplaceText.isEmpty())
                ReplaceTextWithSynonym( pOLV->GetEditView(), aReplaceText );
            break;
        }

        case SID_ATTR_CHAR_FONT:
        case SID_ATTR_CHAR_FONTHEIGHT:
        case SID_ATTR_CHAR_WEIGHT:
        case SID_ATTR_CHAR_POSTURE:
        {
            SfxItemPool* pPool2 = aEditAttr.GetPool()->GetSecondaryPool();
            if( !pPool2 )
                pPool2 = aEditAttr.GetPool();
            SvxScriptSetItem aSetItem( nSlot, *pPool2 );

            // #i78017 establish the same behaviour as in Writer
            SvtScriptType nScriptTypes = SvtScriptType::LATIN | SvtScriptType::ASIAN | SvtScriptType::COMPLEX;
            if (nSlot == SID_ATTR_CHAR_FONT)
                nScriptTypes = pOLV->GetSelectedScriptType();

            if (pNewAttrs)
            {
                aSetItem.PutItemForScriptType( nScriptTypes, pNewAttrs->Get( nWhich ) );
                aNewAttr.Put( aSetItem.GetItemSet() );
            }
        }
        break;

        case SID_ATTR_CHAR_COLOR: nEEWhich = EE_CHAR_COLOR; break;

        case SID_ATTR_CHAR_COLOR2:
        {
            if (!rReq.GetArgs())
            {
                const std::optional<NamedColor> oColor
                    = GetView().GetDocShell()->GetRecentColor(SID_ATTR_CHAR_COLOR);
                if (oColor.has_value())
                {
                    nEEWhich = GetPool().GetWhichIDFromSlotID(SID_ATTR_CHAR_COLOR);
                    const model::ComplexColor aCol = (*oColor).getComplexColor();
                    aNewAttr.Put(SvxColorItem(aCol.getFinalColor(), aCol, nEEWhich));
                    rReq.SetArgs(aNewAttr);
                    rReq.SetSlot(SID_ATTR_CHAR_COLOR);
                }
            }
        }
        break;
        case SID_ATTR_CHAR_BACK_COLOR:
        {
            nEEWhich = GetPool().GetWhichIDFromSlotID(nSlot);
            if (!rReq.GetArgs())
            {
                const std::optional<NamedColor> oColor
                    = m_rView.GetDocShell()->GetRecentColor(nSlot);
                if (oColor.has_value())
                {
                    const model::ComplexColor aCol = (*oColor).getComplexColor();
                    aNewAttr.Put(SvxColorItem(aCol.getFinalColor(), aCol, nEEWhich));
                }
            }
            break;
        }
        case SID_ATTR_CHAR_UNDERLINE:
        {
            if ( pNewAttrs )
            {
                const SvxTextLineItem& rTextLineItem = static_cast< const SvxTextLineItem& >( pNewAttrs->Get( pNewAttrs->GetPool()->GetWhichIDFromSlotID(nSlot) ) );
                aNewAttr.Put( SvxUnderlineItem( rTextLineItem.GetLineStyle(), EE_CHAR_UNDERLINE ) );
            }
            else
            {
                FontLineStyle eFU = aEditAttr.Get(EE_CHAR_UNDERLINE).GetLineStyle();
                aNewAttr.Put( SvxUnderlineItem(eFU == LINESTYLE_SINGLE ? LINESTYLE_NONE : LINESTYLE_SINGLE, EE_CHAR_UNDERLINE) );
            }
        }
        break;

        case SID_ATTR_CHAR_OVERLINE:
        {
            FontLineStyle eFO = aEditAttr.Get(EE_CHAR_OVERLINE).GetLineStyle();
            aNewAttr.Put(SvxOverlineItem(eFO == LINESTYLE_SINGLE ? LINESTYLE_NONE : LINESTYLE_SINGLE, EE_CHAR_OVERLINE));
        }
        break;

        case SID_ATTR_CHAR_CONTOUR:     nEEWhich = EE_CHAR_OUTLINE; break;
        case SID_ATTR_CHAR_SHADOWED:    nEEWhich = EE_CHAR_SHADOW; break;
        case SID_ATTR_CHAR_STRIKEOUT:   nEEWhich = EE_CHAR_STRIKEOUT; break;
        case SID_ATTR_CHAR_WORDLINEMODE: nEEWhich = EE_CHAR_WLM; break;
        case SID_ATTR_CHAR_RELIEF      : nEEWhich = EE_CHAR_RELIEF;  break;
        case SID_ATTR_CHAR_LANGUAGE    : nEEWhich = EE_CHAR_LANGUAGE;break;
        case SID_ATTR_CHAR_KERNING     : nEEWhich = EE_CHAR_KERNING; break;
        case SID_ATTR_CHAR_SCALEWIDTH:   nEEWhich = EE_CHAR_FONTWIDTH; break;
        case SID_ATTR_CHAR_AUTOKERN  :   nEEWhich = EE_CHAR_PAIRKERNING; break;
        case SID_ATTR_CHAR_ESCAPEMENT:   nEEWhich = EE_CHAR_ESCAPEMENT; break;
        case SID_ATTR_PARA_ADJUST_LEFT:
            aNewAttr.Put(SvxAdjustItem(SvxAdjust::Left, EE_PARA_JUST));
        break;
        case SID_ATTR_PARA_ADJUST_CENTER:
            aNewAttr.Put(SvxAdjustItem(SvxAdjust::Center, EE_PARA_JUST));
        break;
        case SID_ATTR_PARA_ADJUST_RIGHT:
            aNewAttr.Put(SvxAdjustItem(SvxAdjust::Right, EE_PARA_JUST));
        break;
        case SID_ATTR_PARA_ADJUST_BLOCK:
            aNewAttr.Put(SvxAdjustItem(SvxAdjust::Block, EE_PARA_JUST));
        break;
        case SID_ATTR_PARA_LRSPACE:
            {
                SvxLRSpaceItem aParaMargin(static_cast<const SvxLRSpaceItem&>(rReq.
                                        GetArgs()->Get(nSlot)));
                aParaMargin.SetWhich( EE_PARA_LRSPACE );
                aNewAttr.Put(aParaMargin);
                rReq.Done();
            }
            break;
        case SID_HANGING_INDENT:
            {
                SfxItemState eState = aEditAttr.GetItemState( EE_PARA_LRSPACE );
                if( eState >= SfxItemState::DEFAULT )
                {
                    SvxLRSpaceItem aParaMargin = aEditAttr.Get( EE_PARA_LRSPACE );
                    aParaMargin.SetWhich( EE_PARA_LRSPACE );

                    tools::Long nIndentDist = aParaMargin.ResolveTextFirstLineOffset({});

                    if (nIndentDist == 0)
                    {
                        const SvxTabStopItem& rDefTabItem = rSh.GetDefault(RES_PARATR_TABSTOP);
                        nIndentDist = ::GetTabDist(rDefTabItem);
                    }

                    aParaMargin.SetTextLeft(
                        SvxIndentValue::twips(aParaMargin.ResolveTextLeft({}) + nIndentDist));
                    aParaMargin.SetRight(aParaMargin.GetRight());
                    aParaMargin.SetTextFirstLineOffset(SvxIndentValue::twips(nIndentDist * -1));

                    aNewAttr.Put(aParaMargin);
                    rReq.Done();
                }
            }
        break;
        case SID_ATTR_PARA_LINESPACE:
            if (pNewAttrs)
            {
                SvxLineSpacingItem aLineSpace = static_cast<const SvxLineSpacingItem&>(pNewAttrs->Get(
                                                            GetPool().GetWhichIDFromSlotID(nSlot)));
                aLineSpace.SetWhich( EE_PARA_SBL );
                aNewAttr.Put( aLineSpace );
                rReq.Done();
            }
            break;
        case SID_ATTR_PARA_ULSPACE:
            if (pNewAttrs)
            {
                SvxULSpaceItem aULSpace = static_cast<const SvxULSpaceItem&>(pNewAttrs->Get(
                    GetPool().GetWhichIDFromSlotID(nSlot)));
                aULSpace.SetWhich( EE_PARA_ULSPACE );
                aNewAttr.Put( aULSpace );
                rReq.Done();
            }
            break;
        case SID_PARASPACE_INCREASE:
        case SID_PARASPACE_DECREASE:
        {
            SvxULSpaceItem aULSpace( aEditAttr.Get( EE_PARA_ULSPACE ) );
            sal_uInt16 nUpper = aULSpace.GetUpper();
            sal_uInt16 nLower = aULSpace.GetLower();

            if ( nSlot == SID_PARASPACE_INCREASE )
            {
                nUpper = std::min< sal_uInt16 >( nUpper + 57, 5670 );
                nLower = std::min< sal_uInt16 >( nLower + 57, 5670 );
            }
            else
            {
                nUpper = std::max< sal_Int16 >( nUpper - 57, 0 );
                nLower = std::max< sal_Int16 >( nLower - 57, 0 );
            }

            aULSpace.SetUpper( nUpper );
            aULSpace.SetLower( nLower );
            aNewAttr.Put( aULSpace );
            rReq.Done();
        }
        break;
        case SID_ATTR_PARA_LINESPACE_10:
        {
            SvxLineSpacingItem aItem(LINE_SPACE_DEFAULT_HEIGHT, EE_PARA_SBL);
            aItem.SetPropLineSpace(100);
            aNewAttr.Put(aItem);
        }
        break;
        case SID_ATTR_PARA_LINESPACE_115:
        {
            SvxLineSpacingItem aItem(LINE_SPACE_DEFAULT_HEIGHT, EE_PARA_SBL);
            aItem.SetPropLineSpace(115);
            aNewAttr.Put(aItem);
        }
        break;
        case SID_ATTR_PARA_LINESPACE_15:
        {
            SvxLineSpacingItem aItem(LINE_SPACE_DEFAULT_HEIGHT, EE_PARA_SBL);
            aItem.SetPropLineSpace(150);
            aNewAttr.Put(aItem);
        }
        break;
        case SID_ATTR_PARA_LINESPACE_20:
        {
            SvxLineSpacingItem aItem(LINE_SPACE_DEFAULT_HEIGHT, EE_PARA_SBL);
            aItem.SetPropLineSpace(200);
            aNewAttr.Put(aItem);
        }
        break;

        case FN_SET_SUPER_SCRIPT:
        {
            SvxEscapementItem aItem(EE_CHAR_ESCAPEMENT);
            SvxEscapement eEsc = aEditAttr.Get(EE_CHAR_ESCAPEMENT).GetEscapement();

            if( eEsc == SvxEscapement::Superscript )
                aItem.SetEscapement( SvxEscapement::Off );
            else
                aItem.SetEscapement( SvxEscapement::Superscript );
            aNewAttr.Put( aItem );
        }
        break;
        case FN_SET_SUB_SCRIPT:
        {
            SvxEscapementItem aItem(EE_CHAR_ESCAPEMENT);
            SvxEscapement eEsc = aEditAttr.Get(EE_CHAR_ESCAPEMENT).GetEscapement();

            if( eEsc == SvxEscapement::Subscript )
                aItem.SetEscapement( SvxEscapement::Off );
            else
                aItem.SetEscapement( SvxEscapement::Subscript );
            aNewAttr.Put( aItem );
        }
        break;

        case SID_CHAR_DLG_EFFECT:
        case SID_CHAR_DLG_POSITION:
        case SID_CHAR_DLG:
        case SID_CHAR_DLG_FOR_PARAGRAPH:
        {
            const SfxItemSet* pArgs = rReq.GetArgs();
            const SfxStringItem* pItem = rReq.GetArg<SfxStringItem>(FN_PARAM_1);

            if( !pArgs || pItem )
            {
                aOldSelection = pOLV->GetSelection();
                if (nSlot == SID_CHAR_DLG_FOR_PARAGRAPH)
                {
                    // select current paragraph (and restore selection later on...)
                    EditView & rEditView = pOLV->GetEditView();
                    SwLangHelper::SelectPara( rEditView, rEditView.GetSelection() );
                    bRestoreSelection = true;
                }

                SwView& rView = GetView();
                FieldUnit eMetric = ::GetDfltMetric(dynamic_cast<SwWebView*>(&rView) != nullptr);
                SwModule::get()->PutItem(SfxUInt16Item(SID_ATTR_METRIC, static_cast< sal_uInt16 >(eMetric)) );
                SfxItemSetFixed<XATTR_FILLSTYLE, XATTR_FILLCOLOR, EE_ITEMS_START, EE_ITEMS_END> aDlgAttr(GetPool());

                // util::Language does not exists in the EditEngine! That is why not in set.

                aDlgAttr.Put( aEditAttr );
                aDlgAttr.Put( SvxKerningItem(0, RES_CHRATR_KERNING) );

                SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
                VclPtr<SfxAbstractTabDialog> pDlg(pFact->CreateSwCharDlg(rView.GetFrameWeld(), rView, aDlgAttr, SwCharDlgMode::Draw));
                if (nSlot == SID_CHAR_DLG_EFFECT)
                {
                    pDlg->SetCurPageId(u"fonteffects"_ustr);
                }
                else if (nSlot == SID_CHAR_DLG_POSITION)
                {
                    pDlg->SetCurPageId(u"position"_ustr);
                }
                else if (nSlot == SID_CHAR_DLG_FOR_PARAGRAPH)
                {
                    pDlg->SetCurPageId(u"font"_ustr);
                }
                else if (pItem)
                {
                    pDlg->SetCurPageId(pItem->GetValue());
                }

                auto xRequest = std::make_shared<SfxRequest>(rReq);
                rReq.Ignore(); // the 'old' request is not relevant any more
                pDlg->StartExecuteAsync(
                    [this, pDlg, xRequest=std::move(xRequest), nEEWhich, aNewAttr2=aNewAttr, pOLV, bRestoreSelection, aOldSelection] (sal_Int32 nResult) mutable ->void
                    {
                        if (nResult == RET_OK)
                        {
                            xRequest->Done( *( pDlg->GetOutputItemSet() ) );
                            aNewAttr2.Put(*pDlg->GetOutputItemSet());
                            ExecutePost(*xRequest, nEEWhich, aNewAttr2, pOLV, bRestoreSelection, aOldSelection);
                        }
                        pDlg->disposeOnce();
                    }
                );
                return;
            }
            else
                aNewAttr.Put(*pArgs);
        }
        break;
        case FN_FORMAT_FOOTNOTE_DLG:
        {
            GetView().ExecFormatFootnote();
            break;
        }
        case FN_NUMBERING_OUTLINE_DLG:
        {
            GetView().ExecNumberingOutline(GetPool());
            rReq.Done();
        }
        break;
        case SID_OPEN_XML_FILTERSETTINGS:
        {
            HandleOpenXmlFilterSettings(rReq);
        }
        break;
        case FN_WORDCOUNT_DIALOG:
        {
            GetView().UpdateWordCount(this, nSlot);
        }
        break;
        case SID_PARA_DLG:
        {
            const SfxItemSet* pArgs = rReq.GetArgs();

            if (!pArgs)
            {
                SwView& rView = GetView();
                FieldUnit eMetric = ::GetDfltMetric(dynamic_cast<SwWebView*>(&rView) !=  nullptr);
                SwModule::get()->PutItem(SfxUInt16Item(SID_ATTR_METRIC, static_cast< sal_uInt16 >(eMetric)) );
                SfxItemSetFixed<
                        EE_ITEMS_START, EE_ITEMS_END,
                        SID_ATTR_PARA_HYPHENZONE, SID_ATTR_PARA_WIDOWS>  aDlgAttr( GetPool() );

                aDlgAttr.Put(aEditAttr);

                aDlgAttr.Put( SvxHyphenZoneItem( false, RES_PARATR_HYPHENZONE) );
                aDlgAttr.Put( SvxFormatBreakItem( SvxBreak::NONE, RES_BREAK ) );
                aDlgAttr.Put( SvxFormatSplitItem( true, RES_PARATR_SPLIT ) );
                aDlgAttr.Put( SvxWidowsItem( 0, RES_PARATR_WIDOWS ) );
                aDlgAttr.Put( SvxOrphansItem( 0, RES_PARATR_ORPHANS ) );

                SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
                ScopedVclPtr<SfxAbstractTabDialog> pDlg(pFact->CreateSwParaDlg(GetView().GetFrameWeld(), GetView(), aDlgAttr, true));
                sal_uInt16 nRet = pDlg->Execute();
                if(RET_OK == nRet)
                {
                    rReq.Done( *( pDlg->GetOutputItemSet() ) );
                    aNewAttr.Put(*pDlg->GetOutputItemSet());
                }
                if(RET_OK != nRet)
                    return;
            }
            else
                aNewAttr.Put(*pArgs);
        }
        break;
        case SID_AUTOSPELL_CHECK:
        {
//!! JP 16.03.2001: why??           pSdrView = rSh.GetDrawView();
//!! JP 16.03.2001: why??           pOutliner = pSdrView->GetTextEditOutliner();
            SdrOutliner * pOutliner = m_pSdrView->GetTextEditOutliner();
            EEControlBits nCtrl = pOutliner->GetControlWord();

            bool bSet = static_cast<const SfxBoolItem&>(rReq.GetArgs()->Get(
                                                    nSlot)).GetValue();
            if(bSet)
                nCtrl |= EEControlBits::ONLINESPELLING|EEControlBits::ALLOWBIGOBJS;
            else
                nCtrl &= ~EEControlBits::ONLINESPELLING;
            pOutliner->SetControlWord(nCtrl);

            m_rView.ExecuteSlot(rReq);
        }
        break;
        case SID_HYPERLINK_SETLINK:
        {
            const SfxPoolItem* pItem = nullptr;
            if(pNewAttrs)
                pNewAttrs->GetItemState(nSlot, false, &pItem);

            if(pItem)
            {
                const SvxHyperlinkItem& rHLinkItem = *static_cast<const SvxHyperlinkItem *>(pItem);
                SvxURLField aField(rHLinkItem.GetURL(), rHLinkItem.GetName(), SvxURLFormat::AppDefault);
                aField.SetTargetFrame(rHLinkItem.GetTargetFrame());

                const SvxFieldItem* pFieldItem = pOLV->GetFieldAtSelection();

                if (pFieldItem && dynamic_cast< const SvxURLField *>( pFieldItem->GetField() ) != nullptr )
                {
                    // Select field so that it will be deleted during insert
                    ESelection aSel = pOLV->GetSelection();
                    aSel.end.nIndex++;
                    pOLV->SetSelection(aSel);
                }
                pOLV->InsertField(SvxFieldItem(aField, EE_FEATURE_FIELD));
            }
        }
        break;

        case SID_EDIT_HYPERLINK:
        {
            // Ensure the field is selected first
            pOLV->SelectFieldAtCursor();
            GetView().GetViewFrame().GetDispatcher()->Execute(SID_HYPERLINK_DIALOG);
        }
        break;

        case SID_REMOVE_HYPERLINK:
        {
            URLFieldHelper::RemoveURLField(pOLV->GetEditView());
        }
        break;

        case SID_OPEN_HYPERLINK:
        {
            const SvxFieldItem* pFieldItem
                = pOLV->GetFieldAtSelection(/*AlsoCheckBeforeCursor=*/true);
            const SvxFieldData* pField = pFieldItem ? pFieldItem->GetField() : nullptr;
            if (const SvxURLField* pURLField = dynamic_cast<const SvxURLField*>(pField))
            {
                ::LoadURL(GetShell(), pURLField->GetURL(), LoadUrlFlags::NONE,
                          pURLField->GetTargetFrame());
            }
        }
        break;

        case SID_COPY_HYPERLINK_LOCATION:
        {
            const SvxFieldItem* pFieldItem
                = pOLV->GetFieldAtSelection(/*AlsoCheckBeforeCursor=*/true);
            const SvxFieldData* pField = pFieldItem ? pFieldItem->GetField() : nullptr;
            if (const SvxURLField* pURLField = dynamic_cast<const SvxURLField*>(pField))
            {
                uno::Reference<datatransfer::clipboard::XClipboard> xClipboard
                    = GetView().GetEditWin().GetClipboard();
                vcl::unohelper::TextDataObject::CopyStringTo(pURLField->GetURL(), xClipboard);
            }
        }
        break;

        case SID_TEXTDIRECTION_LEFT_TO_RIGHT:
        case SID_TEXTDIRECTION_TOP_TO_BOTTOM:
            // Shell switch!
            {
                SdrObject* pTmpObj = m_pSdrView->GetMarkedObjectList().GetMark(0)->GetMarkedSdrObj();
                SdrPageView* pTmpPV = m_pSdrView->GetSdrPageView();
                SdrView* pTmpView = m_pSdrView;

                m_pSdrView->SdrEndTextEdit(true);

                SfxItemSetFixed<SDRATTR_TEXTDIRECTION,
                            SDRATTR_TEXTDIRECTION>  aAttr( *aNewAttr.GetPool() );

                aAttr.Put( SvxWritingModeItem(
                    nSlot == SID_TEXTDIRECTION_LEFT_TO_RIGHT ?
                        text::WritingMode_LR_TB
                        : text::WritingMode_TB_RL, SDRATTR_TEXTDIRECTION ) );
                pTmpView->SetAttributes( aAttr );

                rSh.GetView().BeginTextEdit( pTmpObj, pTmpPV, &rSh.GetView().GetEditWin());
                rSh.GetView().AttrChangedNotify(nullptr);
            }
            return;

        case SID_ATTR_PARA_LEFT_TO_RIGHT:
        case SID_ATTR_PARA_RIGHT_TO_LEFT:
        {
            SdrObject* pTmpObj = m_pSdrView->GetMarkedObjectList().GetMark(0)->GetMarkedSdrObj();
            SdrPageView* pTmpPV = m_pSdrView->GetSdrPageView();
            SdrView* pTmpView = m_pSdrView;

            m_pSdrView->SdrEndTextEdit(true);
            bool bLeftToRight = nSlot == SID_ATTR_PARA_LEFT_TO_RIGHT;

            const SfxPoolItem* pPoolItem;
            if( pNewAttrs && SfxItemState::SET == pNewAttrs->GetItemState( nSlot, true, &pPoolItem ) )
            {
                if( !static_cast<const SfxBoolItem*>(pPoolItem)->GetValue() )
                    bLeftToRight = !bLeftToRight;
            }
            SfxItemSetFixed<
                    EE_PARA_WRITINGDIR, EE_PARA_WRITINGDIR,
                    EE_PARA_JUST, EE_PARA_JUST>  aAttr( *aNewAttr.GetPool() );

            SvxAdjust nAdjust = SvxAdjust::Left;
            if( const SvxAdjustItem* pAdjustItem = aEditAttr.GetItemIfSet(EE_PARA_JUST) )
                nAdjust = pAdjustItem->GetAdjust();

            if( bLeftToRight )
            {
                aAttr.Put( SvxFrameDirectionItem( SvxFrameDirection::Horizontal_LR_TB, EE_PARA_WRITINGDIR ) );
                if( nAdjust == SvxAdjust::Right )
                    aAttr.Put( SvxAdjustItem( SvxAdjust::Left, EE_PARA_JUST ) );
            }
            else
            {
                aAttr.Put( SvxFrameDirectionItem( SvxFrameDirection::Horizontal_RL_TB, EE_PARA_WRITINGDIR ) );
                if( nAdjust == SvxAdjust::Left )
                    aAttr.Put( SvxAdjustItem( SvxAdjust::Right, EE_PARA_JUST ) );
            }
            pTmpView->SetAttributes( aAttr );
            rSh.GetView().BeginTextEdit( pTmpObj, pTmpPV, &rSh.GetView().GetEditWin() );
            rSh.GetView().AttrChangedNotify(nullptr);
        }
        return;

        case FN_GROW_FONT_SIZE:
        case FN_SHRINK_FONT_SIZE:
        {
            const SfxObjectShell* pObjSh = SfxObjectShell::Current();
            const SvxFontListItem* pFontListItem = static_cast< const SvxFontListItem* >
                    (pObjSh ? pObjSh->GetItem(SID_ATTR_CHAR_FONTLIST) : nullptr);
            const FontList* pFontList = pFontListItem ? pFontListItem->GetFontList() : nullptr;
            pOLV->GetEditView().ChangeFontSize( nSlot == FN_GROW_FONT_SIZE, pFontList );
        }
        break;

        default:
            assert(false && "wrong dispatcher");
            return;
    }

    ExecutePost(rReq, nEEWhich, aNewAttr, pOLV, bRestoreSelection, aOldSelection);
}

void SwDrawTextShell::ExecutePost( const SfxRequest& rReq, sal_uInt16 nEEWhich, SfxItemSet& rNewAttr,
        const OutlinerView* pOLV, bool bRestoreSelection, const ESelection& rOldSelection )
{
    SwWrtShell &rSh = GetShell();
    const SfxItemSet *pNewAttrs = rReq.GetArgs();
    const sal_uInt16 nSlot = rReq.GetSlot();
    const sal_uInt16 nWhich = GetPool().GetWhichIDFromSlotID(nSlot);

    if (nEEWhich && pNewAttrs)
    {
        rNewAttr.Put(pNewAttrs->Get(nWhich).CloneSetWhich(nEEWhich));
    }
    else if (nEEWhich == EE_CHAR_COLOR)
    {
        GetView().GetViewFrame().GetDispatcher()->Execute(SID_CHAR_DLG_EFFECT);
    }
    else if (nEEWhich == EE_CHAR_KERNING)
    {
        GetView().GetViewFrame().GetDispatcher()->Execute(SID_CHAR_DLG_POSITION);
    }


    SetAttrToMarked(rNewAttr);

    GetView().GetViewFrame().GetBindings().InvalidateAll(false);

    if (IsTextEdit() && pOLV->GetOutliner().IsModified())
        rSh.SetModified();

    if (bRestoreSelection)
    {
        // restore selection
        pOLV->GetEditView().SetSelection( rOldSelection );
    }
}

void SwDrawTextShell::GetState(SfxItemSet& rSet)
{
    if (!IsTextEdit()) // Otherwise sometimes crash!
        return;

    OutlinerView* pOLV = m_pSdrView->GetTextEditOutlinerView();
    SfxWhichIter aIter(rSet);
    sal_uInt16 nWhich = aIter.FirstWhich();

    SfxItemSet aEditAttr(pOLV->GetAttribs());
    const SvxAdjustItem *pAdjust = nullptr;
    const SvxLineSpacingItem *pLSpace = nullptr;
    const SfxPoolItem *pEscItem = nullptr;
    SvxAdjust eAdjust;
    int nLSpace;
    SvxEscapement nEsc;

    while (nWhich)
    {
        sal_uInt16 nSlotId = GetPool().GetSlotId(nWhich);
        bool bFlag = false;
        switch (nSlotId)
        {
            case SID_LANGUAGE_STATUS: //20412:
            {
                SwLangHelper::GetLanguageStatus(pOLV, rSet);
                nSlotId = 0;
                break;
            }

            case SID_THES:
            {
                OUString aStatusVal;
                LanguageType nLang = LANGUAGE_NONE;
                bool bIsLookUpWord
                    = GetStatusValueForThesaurusFromContext(aStatusVal, nLang, pOLV->GetEditView());
                rSet.Put(SfxStringItem(SID_THES, aStatusVal));

                // disable "Thesaurus" context menu entry if there is nothing to look up
                uno::Reference<linguistic2::XThesaurus> xThes(::GetThesaurus());
                if (!bIsLookUpWord || !xThes.is() || nLang == LANGUAGE_NONE
                    || !xThes->hasLocale(LanguageTag::convertToLocale(nLang)))
                    rSet.DisableItem(SID_THES);

                //! avoid putting the same item as SfxBoolItem at the end of this function
                nSlotId = 0;
                break;
            }

            case SID_ATTR_PARA_ADJUST_LEFT:
                eAdjust = SvxAdjust::Left;
                goto ASK_ADJUST;
            case SID_ATTR_PARA_ADJUST_RIGHT:
                eAdjust = SvxAdjust::Right;
                goto ASK_ADJUST;
            case SID_ATTR_PARA_ADJUST_CENTER:
                eAdjust = SvxAdjust::Center;
                goto ASK_ADJUST;
            case SID_ATTR_PARA_ADJUST_BLOCK:
                eAdjust = SvxAdjust::Block;
                goto ASK_ADJUST;
            ASK_ADJUST:
            {
                if (!pAdjust)
                    pAdjust = aEditAttr.GetItemIfSet(EE_PARA_JUST, false);

                if (!pAdjust || IsInvalidItem(pAdjust))
                {
                    rSet.InvalidateItem(nSlotId);
                    nSlotId = 0;
                }
                else
                    bFlag = eAdjust == pAdjust->GetAdjust();
            }
            break;

            case SID_ATTR_PARA_LRSPACE:
            case SID_ATTR_PARA_LEFTSPACE:
            case SID_ATTR_PARA_RIGHTSPACE:
            case SID_ATTR_PARA_FIRSTLINESPACE:
            {
                SfxItemState eState = aEditAttr.GetItemState(EE_PARA_LRSPACE);
                if (eState >= SfxItemState::DEFAULT)
                {
                    SvxLRSpaceItem aLR = aEditAttr.Get(EE_PARA_LRSPACE);
                    aLR.SetWhich(SID_ATTR_PARA_LRSPACE);
                    rSet.Put(aLR);
                }
                else
                    rSet.InvalidateItem(nSlotId);
                nSlotId = 0;
            }
            break;
            case SID_ATTR_PARA_LINESPACE:
            {
                SfxItemState eState = aEditAttr.GetItemState(EE_PARA_SBL);
                if (eState >= SfxItemState::DEFAULT)
                {
                    const SvxLineSpacingItem& aLR = aEditAttr.Get(EE_PARA_SBL);
                    rSet.Put(aLR);
                }
                else
                    rSet.InvalidateItem(nSlotId);
                nSlotId = 0;
            }
            break;
            case SID_ATTR_PARA_ULSPACE:
            case SID_ATTR_PARA_BELOWSPACE:
            case SID_ATTR_PARA_ABOVESPACE:
            case SID_PARASPACE_INCREASE:
            case SID_PARASPACE_DECREASE:
            {
                SfxItemState eState = aEditAttr.GetItemState(EE_PARA_ULSPACE);
                if (eState >= SfxItemState::DEFAULT)
                {
                    SvxULSpaceItem aULSpace = aEditAttr.Get(EE_PARA_ULSPACE);
                    if (!aULSpace.GetUpper() && !aULSpace.GetLower())
                        rSet.DisableItem(SID_PARASPACE_DECREASE);
                    else if (aULSpace.GetUpper() >= 5670 && aULSpace.GetLower() >= 5670)
                        rSet.DisableItem(SID_PARASPACE_INCREASE);
                    if (nSlotId == SID_ATTR_PARA_ULSPACE || nSlotId == SID_ATTR_PARA_ABOVESPACE
                        || nSlotId == SID_ATTR_PARA_BELOWSPACE)
                    {
                        aULSpace.SetWhich(nSlotId);
                        rSet.Put(aULSpace);
                    }
                }
                else
                {
                    rSet.DisableItem(SID_PARASPACE_INCREASE);
                    rSet.DisableItem(SID_PARASPACE_DECREASE);
                    rSet.InvalidateItem(SID_ATTR_PARA_ULSPACE);
                    rSet.InvalidateItem(SID_ATTR_PARA_ABOVESPACE);
                    rSet.InvalidateItem(SID_ATTR_PARA_BELOWSPACE);
                }
                nSlotId = 0;
            }
            break;

            case SID_ATTR_PARA_LINESPACE_10:
                nLSpace = 100;
                goto ASK_LINESPACE;
            case SID_ATTR_PARA_LINESPACE_115:
                nLSpace = 115;
                goto ASK_LINESPACE;
            case SID_ATTR_PARA_LINESPACE_15:
                nLSpace = 150;
                goto ASK_LINESPACE;
            case SID_ATTR_PARA_LINESPACE_20:
                nLSpace = 200;
                goto ASK_LINESPACE;
            ASK_LINESPACE:
            {
                if (!pLSpace)
                    pLSpace = aEditAttr.GetItemIfSet(EE_PARA_SBL, false);

                if (!pLSpace || IsInvalidItem(pLSpace))
                {
                    rSet.InvalidateItem(nSlotId);
                    nSlotId = 0;
                }
                else if (nLSpace
                         == pLSpace->GetPropLineSpace())
                    bFlag = true;
                else
                {
                    // tdf#114631 - disable non selected line spacing
                    rSet.Put(SfxBoolItem(nWhich, false));
                    nSlotId = 0;
                }
            }
            break;

            case FN_SET_SUPER_SCRIPT:
                nEsc = SvxEscapement::Superscript;
                goto ASK_ESCAPE;
            case FN_SET_SUB_SCRIPT:
                nEsc = SvxEscapement::Subscript;
                goto ASK_ESCAPE;
            ASK_ESCAPE:
            {
                if (!pEscItem)
                    pEscItem = &aEditAttr.Get(EE_CHAR_ESCAPEMENT);

                if (nEsc == static_cast<const SvxEscapementItem*>(pEscItem)->GetEscapement())
                    bFlag = true;
                else
                    nSlotId = 0;
            }
            break;

            case SID_THESAURUS:
            {
                // disable "Thesaurus" if the language is not supported
                TypedWhichId<SvxLanguageItem> nLangWhich = GetWhichOfScript(
                    RES_CHRATR_LANGUAGE,
                    SvtLanguageOptions::GetI18NScriptTypeOfLanguage(GetAppLanguage()));
                const SvxLanguageItem& rItem = GetShell().GetDoc()->GetDefault(nLangWhich);
                LanguageType nLang = rItem.GetLanguage();

                uno::Reference<linguistic2::XThesaurus> xThes(::GetThesaurus());
                if (!xThes.is() || nLang == LANGUAGE_NONE
                    || !xThes->hasLocale(LanguageTag::convertToLocale(nLang)))
                    rSet.DisableItem(SID_THESAURUS);
                nSlotId = 0;
            }
            break;
            case SID_HANGUL_HANJA_CONVERSION:
            case SID_CHINESE_CONVERSION:
            {
                if (!SvtCJKOptions::IsAnyEnabled())
                {
                    GetView().GetViewFrame().GetBindings().SetVisibleState(nWhich, false);
                    rSet.DisableItem(nWhich);
                }
                else
                    GetView().GetViewFrame().GetBindings().SetVisibleState(nWhich, true);
            }
            break;

            case SID_TEXTDIRECTION_LEFT_TO_RIGHT:
            case SID_TEXTDIRECTION_TOP_TO_BOTTOM:
                if (!SvtCJKOptions::IsVerticalTextEnabled())
                {
                    rSet.DisableItem(nSlotId);
                    nSlotId = 0;
                }
                else
                {
                    SdrOutliner* pOutliner = m_pSdrView->GetTextEditOutliner();
                    if (pOutliner)
                        bFlag = pOutliner->IsVertical()
                                == (SID_TEXTDIRECTION_TOP_TO_BOTTOM == nSlotId);
                    else
                    {
                        text::WritingMode eMode = aEditAttr.Get(SDRATTR_TEXTDIRECTION).GetValue();

                        if (nSlotId == SID_TEXTDIRECTION_LEFT_TO_RIGHT)
                        {
                            bFlag = eMode == text::WritingMode_LR_TB;
                        }
                        else
                        {
                            bFlag = eMode != text::WritingMode_TB_RL;
                        }
                    }
                }
                break;
            case SID_ATTR_PARA_LEFT_TO_RIGHT:
            case SID_ATTR_PARA_RIGHT_TO_LEFT:
            {
                if (!SvtCTLOptions::IsCTLFontEnabled())
                {
                    rSet.DisableItem(nWhich);
                    nSlotId = 0;
                }
                else
                {
                    SdrOutliner* pOutliner = m_pSdrView->GetTextEditOutliner();
                    if (pOutliner && pOutliner->IsVertical())
                    {
                        rSet.DisableItem(nWhich);
                        nSlotId = 0;
                    }
                    else
                    {
                        switch (aEditAttr.Get(EE_PARA_WRITINGDIR).GetValue())
                        {
                            case SvxFrameDirection::Horizontal_LR_TB:
                                bFlag = nWhich == SID_ATTR_PARA_LEFT_TO_RIGHT;
                                break;

                            case SvxFrameDirection::Horizontal_RL_TB:
                                bFlag = nWhich != SID_ATTR_PARA_LEFT_TO_RIGHT;
                                break;
                            default:
                                break;
                        }
                    }
                }
            }
            break;
            case SID_TRANSLITERATE_HALFWIDTH:
            case SID_TRANSLITERATE_FULLWIDTH:
            case SID_TRANSLITERATE_HIRAGANA:
            case SID_TRANSLITERATE_KATAKANA:
            {
                if (!SvtCJKOptions::IsChangeCaseMapEnabled())
                {
                    rSet.DisableItem(nWhich);
                    GetView().GetViewFrame().GetBindings().SetVisibleState(nWhich, false);
                }
                else
                    GetView().GetViewFrame().GetBindings().SetVisibleState(nWhich, true);
            }
            break;
            case SID_INSERT_RLM:
            case SID_INSERT_LRM:
            {
                bool bEnabled = SvtCTLOptions::IsCTLFontEnabled();
                GetView().GetViewFrame().GetBindings().SetVisibleState(nWhich, bEnabled);
                if (!bEnabled)
                    rSet.DisableItem(nWhich);
            }
            break;
            case SID_REMOVE_HYPERLINK:
            case SID_EDIT_HYPERLINK:
            case SID_OPEN_HYPERLINK:
            case SID_COPY_HYPERLINK_LOCATION:
            {
                if (!URLFieldHelper::IsCursorAtURLField(pOLV, /*AlsoCheckBeforeCursor=*/true))
                    rSet.DisableItem(nWhich);
                nSlotId = 0;
            }
            break;
            default:
                nSlotId = 0; // don't know this slot
                break;
        }

        if (nSlotId)
            rSet.Put(SfxBoolItem(nWhich, bFlag));

        nWhich = aIter.NextWhich();
    }
}

void SwDrawTextShell::GetDrawTextCtrlState(SfxItemSet& rSet)
{
    if (!IsTextEdit())  // Otherwise crash!
        return;

    OutlinerView* pOLV = m_pSdrView->GetTextEditOutlinerView();
    SfxItemSet aEditAttr(pOLV->GetAttribs());

    SfxWhichIter aIter(rSet);
    sal_uInt16 nWhich = aIter.FirstWhich();
    SvtScriptType nScriptType = pOLV->GetSelectedScriptType();
    while(nWhich)
    {
        sal_uInt16 nEEWhich = 0;
        sal_uInt16 nSlotId = GetPool().GetSlotId( nWhich );
        switch( nSlotId )
        {
            case SID_ATTR_CHAR_FONT:
            case SID_ATTR_CHAR_FONTHEIGHT:
            case SID_ATTR_CHAR_WEIGHT:
            case SID_ATTR_CHAR_POSTURE:
            {
                SfxItemPool* pEditPool = aEditAttr.GetPool()->GetSecondaryPool();
                if( !pEditPool )
                    pEditPool = aEditAttr.GetPool();
                SvxScriptSetItem aSetItem( nSlotId, *pEditPool );
                aSetItem.GetItemSet().Put( aEditAttr, false );
                const SfxPoolItem* pI = aSetItem.GetItemOfScript( nScriptType );
                if( pI )
                {
                    rSet.Put(pI->CloneSetWhich(nWhich));
                }
                else
                    rSet.InvalidateItem( nWhich );
            }
            break;
            case SID_ATTR_CHAR_COLOR2:
            case SID_ATTR_CHAR_COLOR: nEEWhich = EE_CHAR_COLOR; break;
            case SID_ATTR_CHAR_BACK_COLOR: nEEWhich = EE_CHAR_BKGCOLOR; break;
            case SID_ATTR_CHAR_UNDERLINE: nEEWhich = EE_CHAR_UNDERLINE;break;
            case SID_ATTR_CHAR_OVERLINE: nEEWhich = EE_CHAR_OVERLINE;break;
            case SID_ATTR_CHAR_CONTOUR: nEEWhich = EE_CHAR_OUTLINE; break;
            case SID_ATTR_CHAR_SHADOWED:  nEEWhich = EE_CHAR_SHADOW;break;
            case SID_ATTR_CHAR_STRIKEOUT: nEEWhich = EE_CHAR_STRIKEOUT;break;
            case SID_AUTOSPELL_CHECK:
            {
                const SfxPoolItemHolder aResult(m_rView.GetSlotState(nWhich));
                if (aResult)
                    rSet.Put(SfxBoolItem(nWhich, static_cast<const SfxBoolItem*>(aResult.getItem())->GetValue()));
                else
                    rSet.DisableItem( nWhich );
                break;
            }
            case SID_ATTR_CHAR_WORDLINEMODE: nEEWhich = EE_CHAR_WLM; break;
            case SID_ATTR_CHAR_RELIEF      : nEEWhich = EE_CHAR_RELIEF;  break;
            case SID_ATTR_CHAR_LANGUAGE    : nEEWhich = EE_CHAR_LANGUAGE;break;
            case SID_ATTR_CHAR_KERNING     : nEEWhich = EE_CHAR_KERNING; break;
            case SID_ATTR_CHAR_SCALEWIDTH:   nEEWhich = EE_CHAR_FONTWIDTH;break;
            case SID_ATTR_CHAR_AUTOKERN  :   nEEWhich = EE_CHAR_PAIRKERNING; break;
            case SID_ATTR_CHAR_ESCAPEMENT:   nEEWhich = EE_CHAR_ESCAPEMENT; break;
            case FN_GROW_FONT_SIZE:
            case FN_SHRINK_FONT_SIZE:
            {
                SfxItemPool* pEditPool = aEditAttr.GetPool()->GetSecondaryPool();
                if( !pEditPool )
                    pEditPool = aEditAttr.GetPool();

                SvxScriptSetItem aSetItem( SID_ATTR_CHAR_FONTHEIGHT, *pEditPool );
                aSetItem.GetItemSet().Put( aEditAttr, false );
                const SvxFontHeightItem* pSize( static_cast<const SvxFontHeightItem*>( aSetItem.GetItemOfScript( nScriptType ) ) );

                if( pSize )
                {
                    sal_uInt32 nSize = pSize->GetHeight();
                    if( nSize >= 19998 )
                        rSet.DisableItem( FN_GROW_FONT_SIZE );
                    else if( nSize <= 40 )
                        rSet.DisableItem( FN_SHRINK_FONT_SIZE );
                }
            }
        }
        if(nEEWhich)
        {
            rSet.Put(aEditAttr.Get(nEEWhich).CloneSetWhich(nWhich));
        }

        nWhich = aIter.NextWhich();
    }
}

void SwDrawTextShell::ExecClpbrd(SfxRequest const &rReq)
{
    if (!IsTextEdit())  // Otherwise crash!
        return;

    OutlinerView* pOLV = m_pSdrView->GetTextEditOutlinerView();

    ESelection aSel(pOLV->GetSelection());
    const bool bCopy = aSel.HasRange();
    sal_uInt16 nId = rReq.GetSlot();
    switch( nId )
    {
        case SID_CUT:
            if (bCopy)
                pOLV->Cut();
            return;

        case SID_COPY:
            if (bCopy)
                pOLV->Copy();
            return;

        case SID_PASTE:
            pOLV->PasteSpecial();
            break;

        case SID_PASTE_UNFORMATTED:
            pOLV->Paste();
            break;

        case SID_PASTE_SPECIAL:
        {
            SvxAbstractDialogFactory* pFact = SvxAbstractDialogFactory::Create();
            ScopedVclPtr<SfxAbstractPasteDialog> pDlg(pFact->CreatePasteDialog(GetView().GetEditWin().GetFrameWeld()));

            pDlg->Insert(SotClipboardFormatId::STRING, OUString());
            pDlg->Insert(SotClipboardFormatId::RTF, OUString());
            pDlg->Insert(SotClipboardFormatId::RICHTEXT, OUString());
            pDlg->Insert(SotClipboardFormatId::HTML_SIMPLE, OUString());

            TransferableDataHelper aDataHelper(TransferableDataHelper::CreateFromSystemClipboard(&GetView().GetEditWin()));
            SotClipboardFormatId nFormat = pDlg->GetFormat(aDataHelper.GetTransferable());

            if (nFormat != SotClipboardFormatId::NONE)
            {
                if (nFormat == SotClipboardFormatId::STRING)
                    pOLV->Paste();
                else
                    pOLV->PasteSpecial(nFormat);
            }

            break;
        }

        case SID_CLIPBOARD_FORMAT_ITEMS:
        {
            SotClipboardFormatId nFormat = SotClipboardFormatId::NONE;
            const SfxPoolItem* pItem;
            if (rReq.GetArgs() && rReq.GetArgs()->GetItemState(nId, true, &pItem) == SfxItemState::SET)
            {
                if (const SfxUInt32Item* pUInt32Item = dynamic_cast<const SfxUInt32Item *>(pItem))
                    nFormat = static_cast<SotClipboardFormatId>(pUInt32Item->GetValue());
            }

            if (nFormat != SotClipboardFormatId::NONE)
            {
                if (nFormat == SotClipboardFormatId::STRING)
                    pOLV->Paste();
                else
                    pOLV->PasteSpecial(nFormat);
            }

            break;
        }

        default:
            OSL_FAIL("wrong dispatcher");
            return;
    }
}

void SwDrawTextShell::StateClpbrd(SfxItemSet &rSet)
{
    if (!IsTextEdit())  // Otherwise crash!
        return;

    OutlinerView* pOLV = m_pSdrView->GetTextEditOutlinerView();
    ESelection aSel(pOLV->GetSelection());
    const bool bCopy = aSel.HasRange();

    TransferableDataHelper aDataHelper( TransferableDataHelper::CreateFromSystemClipboard( &GetView().GetEditWin() ) );
    const bool bPaste = aDataHelper.HasFormat( SotClipboardFormatId::STRING ) ||
                        aDataHelper.HasFormat( SotClipboardFormatId::RTF ) ||
                        aDataHelper.HasFormat( SotClipboardFormatId::RICHTEXT ) ||
                        aDataHelper.HasFormat( SotClipboardFormatId::HTML_SIMPLE);

    SfxWhichIter aIter(rSet);
    sal_uInt16 nWhich = aIter.FirstWhich();

    while(nWhich)
    {
        switch(nWhich)
        {
        case SID_CUT:
        case SID_COPY:
            if( !bCopy || GetObjectShell()->isContentExtractionLocked())
                rSet.DisableItem( nWhich );
            break;

        case SID_PASTE:
        case SID_PASTE_UNFORMATTED:
        case SID_PASTE_SPECIAL:
            if( !bPaste )
                rSet.DisableItem( nWhich );
            break;

        case SID_CLIPBOARD_FORMAT_ITEMS:
            if( bPaste )
            {
                SvxClipboardFormatItem aFormats( SID_CLIPBOARD_FORMAT_ITEMS );

                if ( aDataHelper.HasFormat( SotClipboardFormatId::STRING ) )
                    aFormats.AddClipbrdFormat( SotClipboardFormatId::STRING );
                if ( aDataHelper.HasFormat( SotClipboardFormatId::RTF ) )
                    aFormats.AddClipbrdFormat( SotClipboardFormatId::RTF );
                if ( aDataHelper.HasFormat( SotClipboardFormatId::RICHTEXT ) )
                    aFormats.AddClipbrdFormat( SotClipboardFormatId::RICHTEXT );
                if (aDataHelper.HasFormat(SotClipboardFormatId::HTML_SIMPLE))
                    aFormats.AddClipbrdFormat(SotClipboardFormatId::HTML_SIMPLE);

                rSet.Put( aFormats );
            }
            else
                rSet.DisableItem( nWhich );
            break;
        }

        nWhich = aIter.NextWhich();
    }
}

// Hyperlink status

void SwDrawTextShell::StateInsert(SfxItemSet &rSet)
{
    if (!IsTextEdit())  // Otherwise crash!
        return;

    OutlinerView* pOLV = m_pSdrView->GetTextEditOutlinerView();
    SfxWhichIter aIter(rSet);
    sal_uInt16 nWhich = aIter.FirstWhich();

    while(nWhich)
    {
        switch(nWhich)
        {
            case SID_HYPERLINK_GETLINK:
                {
                    SvxHyperlinkItem aHLinkItem;
                    aHLinkItem.SetInsertMode(HLINK_FIELD);

                    const SvxFieldItem* pFieldItem = pOLV->GetFieldAtSelection();

                    if (pFieldItem)
                    {
                        const SvxURLField* pURLField = dynamic_cast<const SvxURLField*>(pFieldItem->GetField());

                        if (pURLField)
                        {
                            aHLinkItem.SetName(pURLField->GetRepresentation());
                            aHLinkItem.SetURL(pURLField->GetURL());
                            aHLinkItem.SetTargetFrame(pURLField->GetTargetFrame());
                        }
                    }
                    else
                    {
                        OUString sSel(pOLV->GetSelected());
                        sSel = sSel.copy(0, std::min<sal_Int32>(255, sSel.getLength()));
                        aHLinkItem.SetName(comphelper::string::stripEnd(sSel, ' '));
                    }

                    sal_uInt16 nHtmlMode = ::GetHtmlMode(GetView().GetDocShell());
                    aHLinkItem.SetInsertMode(static_cast<SvxLinkInsertMode>(aHLinkItem.GetInsertMode() |
                        ((nHtmlMode & HTMLMODE_ON) != 0 ? HLINK_HTMLMODE : 0)));

                    rSet.Put(aHLinkItem);
                }
                break;
        }
        nWhich = aIter.NextWhich();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
