//	CShieldHUDDefault.cpp
//
//	CShieldHUDDefault class
//	Copyright (c) 2015 by Kronosaur Productions, LLC. All Rights Reserved.

#include "stdafx.h"

#define IMAGE_TAG								CONSTLIT("Image")
#define SHIELD_DISPLAY_TAG						CONSTLIT("ShieldDisplay")
#define SHIELD_EFFECT_TAG						CONSTLIT("ShieldLevelEffect")

#define SHIELD_EFFECT_ATTRIB					CONSTLIT("shieldLevelEffect")

#define ERR_SHIELD_DISPLAY_NEEDED				CONSTLIT("missing or invalid <ShieldDisplay> element")

const int DISPLAY_WIDTH = 360;
const int DISPLAY_HEIGHT = 136;
const int SHIELD_IMAGE_WIDTH = 136;
const int SHIELD_IMAGE_HEIGHT = 136;

const int DESCRIPTION_WIDTH = (DISPLAY_WIDTH - SHIELD_IMAGE_WIDTH);
const int SHIELD_HP_DISPLAY_X = DESCRIPTION_WIDTH;
const int SHIELD_HP_DISPLAY_Y = (DISPLAY_HEIGHT - 16);
const int SHIELD_HP_DISPLAY_WIDTH = 26;
const int SHIELD_HP_DISPLAY_HEIGHT = 14;
const CG32bitPixel DISABLED_TEXT_COLOR = CG32bitPixel(128, 0, 0);

CShieldHUDDefault::CShieldHUDDefault (void) :
		m_dwCachedShipID(0),
		m_pShieldPainter(NULL)

//	CShieldHUDDefault constructor

	{
	}

CShieldHUDDefault::~CShieldHUDDefault (void)

//	CShieldHUDDefault destructor

	{
	if (m_pShieldPainter)
		m_pShieldPainter->Delete();
	}

ALERROR CShieldHUDDefault::Bind (SDesignLoadCtx &Ctx)

//	Bind
//
//	Bind design

	{
	ALERROR error;

	if (!m_pShieldEffect.IsEmpty())
		{
		if (error = m_pShieldEffect.Bind(Ctx))
			return error;
		}
	else
		{
		if (error = m_Image.OnDesignLoadComplete(Ctx))
			return error;
		}

	return NOERROR;
	}

void CShieldHUDDefault::GetBounds (int *retWidth, int *retHeight) const

//	GetBounds
//
//	Returns bounds

	{
	*retWidth = DISPLAY_WIDTH;
	*retHeight = DISPLAY_HEIGHT;
	}

bool CShieldHUDDefault::OnCreate (SHUDCreateCtx &CreateCtx, CString *retsError)

//	InitFromXML
//
//	Initializes from XML element

	{
	ALERROR error;
	SDesignLoadCtx Ctx;

	//	Load the new shield effect

	if (error = m_pShieldEffect.LoadEffect(Ctx,
			strPatternSubst(CONSTLIT("%d:p:s"), CreateCtx.Class.GetUNID()),
			CreateCtx.Desc.GetContentElementByTag(SHIELD_EFFECT_TAG),
			CreateCtx.Desc.GetAttribute(SHIELD_EFFECT_ATTRIB)))
		return ComposeLoadError(Ctx.sError, retsError);

	//	If we don't have the new effect, load the backwards compatibility
	//	image.

	if (m_pShieldEffect.IsEmpty())
		{
		if (error = m_Image.InitFromXML(Ctx, 
				*CreateCtx.Desc.GetContentElementByTag(IMAGE_TAG)))
			return ComposeLoadError(ERR_SHIELD_DISPLAY_NEEDED, retsError);
		}

	//	Bind

	if (Bind(Ctx) != NOERROR)
		return ComposeLoadError(Ctx.sError, retsError);

	return true;
	}

void CShieldHUDDefault::OnPaint (CG32bitImage &Dest, int x, int y, SHUDPaintCtx &Ctx)

//	OnPaint
//
//	Paint

	{
	DEBUG_TRY

	int i;

	//	If we're painting over a buffer, then we paint the previously 
	//	initialized text.

	if (Ctx.iMode == SHUDPaintCtx::paintOverBuffer)
		{
		for (i = 0; i < m_Text.GetCount(); i++)
			{
			STextPaint *pPaint = &m_Text[i];
			pPaint->pFont->DrawText(Dest,
					x + pPaint->x,
					y + pPaint->y,
					pPaint->rgbColor,
					pPaint->sText);
			}

		return;
		}

	m_Text.DeleteAll();

	//	Skip if we don't have a ship

	CShip *pShip = Ctx.Source.AsShip();
	if (pShip == NULL)
		return;

	CItemListManipulator ItemList(pShip->GetItemList());

	const CVisualPalette &VI = g_pHI->GetVisuals();
	const CG16bitFont &SmallFont = VI.GetFont(fontSmall);
	const CG16bitFont &MediumFont = VI.GetFont(fontMedium);

	//	If we've changed ships then we need to delete the painters

	if (m_dwCachedShipID != pShip->GetID())
		{
		if (m_pShieldPainter)
			{
			m_pShieldPainter->Delete();
			m_pShieldPainter = NULL;
			}

		m_dwCachedShipID = pShip->GetID();
		}

	//	Figure out the status of the shields

	int iHP = 0;
	int iMaxHP = 10;
	CInstalledDevice *pShield = pShip->GetNamedDevice(devShields);
	if (pShield)
		pShield->GetStatus(pShip, &iHP, &iMaxHP);

	//	Draw the old-style shields

	if (!m_pShieldEffect)
		{
		int iWhole = (iMaxHP > 0 ? (iHP * 100) / iMaxHP : 0);
		int iIndex = (100 - iWhole) / 20;

		const RECT &rcShield = m_Image.GetImageRect();
		Dest.Blt(x + rcShield.left, 
				y + rcShield.top + (RectHeight(rcShield) * iIndex), 
				RectWidth(rcShield), 
				RectHeight(rcShield), 
				255,
				m_Image.GetImage(NULL_STR), 
				DESCRIPTION_WIDTH + ((SHIELD_IMAGE_WIDTH - RectWidth(rcShield)) / 2),
				(SHIELD_IMAGE_HEIGHT - RectHeight(rcShield)) / 2);
		}

	if (pShield)
		{
		CItemCtx ItemCtx(pShip, pShield);

		int cxWidth;

		if (iMaxHP > 0)
			{
			Dest.Fill(x + SHIELD_HP_DISPLAY_X,
					y + SHIELD_HP_DISPLAY_Y, 
					SHIELD_HP_DISPLAY_WIDTH, 
					SHIELD_HP_DISPLAY_HEIGHT,
					VI.GetColor(colorAreaShields));
		
			CString sHP = strFromInt(iHP);
			int cxWidth = MediumFont.MeasureText(sHP, NULL);
			MediumFont.DrawText(Dest,
					x + SHIELD_HP_DISPLAY_X + (SHIELD_HP_DISPLAY_WIDTH - cxWidth) / 2,
					y + SHIELD_HP_DISPLAY_Y - 1,
					VI.GetColor(colorTextShields),
					sHP);
			}

		CGDraw::LineBroken(Dest,
				x,
				y + SHIELD_HP_DISPLAY_Y,
				x + SHIELD_HP_DISPLAY_X,
				y + SHIELD_HP_DISPLAY_Y,
				0,
				VI.GetColor(colorAreaShields));

		CG32bitPixel rgbColor;
		if (pShield->IsWorking())
			rgbColor = VI.GetColor(colorTextShields);
		else
			rgbColor = DISABLED_TEXT_COLOR;

		CString sShieldName = pShield->GetClass()->GetName();
		int cyHeight;
		cxWidth = MediumFont.MeasureText(sShieldName, &cyHeight);

		//	Add the shield name to list of text to paint

		STextPaint *pPaint = m_Text.Insert();
		pPaint->sText = sShieldName;
		pPaint->x = 0;
		pPaint->y = SHIELD_HP_DISPLAY_Y;
		pPaint->pFont = &MediumFont;
		pPaint->rgbColor = rgbColor;

		//	Paint the modifiers

		TArray<SDisplayAttribute> Attribs;
		if (pShield->GetItem()->AccumulateEnhancementDisplayAttributes(Attribs))
			{
			CUIHelper Helper(*g_pHI);

			DWORD dwOptions = CUIHelper::OPTION_ALIGN_RIGHT;

			Helper.PaintDisplayAttribs(Dest, x + SHIELD_HP_DISPLAY_X, y + SHIELD_HP_DISPLAY_Y, Attribs, dwOptions);
			}
		}

	//	Draw the new style shields on top

	if (m_pShieldEffect)
		{
		int x = DESCRIPTION_WIDTH + SHIELD_IMAGE_WIDTH / 2;
		int y = SHIELD_IMAGE_HEIGHT / 2;

		SViewportPaintCtx Ctx;
		Ctx.iTick = g_pUniverse->GetTicks();
		Ctx.iVariant = (iMaxHP > 0 ? (iHP * 100) / iMaxHP : 0);
		Ctx.iRotation = 90;
		Ctx.iDestiny = pShip->GetDestiny();

		if (m_pShieldPainter == NULL)
			{
			CCreatePainterCtx CreateCtx;
			m_pShieldPainter = m_pShieldEffect->CreatePainter(CreateCtx);
			}

		m_pShieldPainter->Paint(Dest, x, y, Ctx);
		}

	DEBUG_CATCH
	}
