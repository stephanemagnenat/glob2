/*
  Copyright (C) 2001-2004 Stephane Magnenat & Luc-Olivier de Charrière
  for any question or comment contact us at nct@ysagoon.com or nuage@ysagoon.com

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <GUIButton.h>
#include <GUIStyle.h>
#include <Toolkit.h>
#include <assert.h>
#include <GraphicContext.h>

using namespace GAGCore;

namespace GAGGUI
{
	Button::Button(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, const char *sprite, int standardId, int highlightID, int returnCode, Uint16 unicodeShortcut)
	:HighlightableWidget(returnCode)
	{
		this->x=x;
		this->y=y;
		this->w=w;
		this->h=h;
		this->hAlignFlag=hAlign;
		this->vAlignFlag=vAlign;
	
		this->unicodeShortcut=unicodeShortcut;
	}

	Button::Button(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, const char *sprite, int standardId, int highlightID, int returnCode,
		const std::string& tooltip, const std::string &tooltipFont, Uint16 unicodeShortcut)
	:HighlightableWidget(tooltip, tooltipFont, returnCode)
	{
		this->x=x;
		this->y=y;
		this->w=w;
		this->h=h;
		this->hAlignFlag=hAlign;
		this->vAlignFlag=vAlign;
	
		this->unicodeShortcut=unicodeShortcut;
	}
	
	void Button::onSDLKeyDown(SDL_Event *event)
	{
		assert(event->type == SDL_KEYDOWN);
		int x, y, w, h;
		getScreenPos(&x, &y, &w, &h);
		Uint16 typedUnicode=event->key.keysym.unicode;
		if ((unicodeShortcut)&&(typedUnicode==unicodeShortcut))
			parent->onAction(this, BUTTON_SHORTCUT, returnCode, unicodeShortcut);
	}

	void Button::onSDLMouseButtonDown(SDL_Event *event)
	{
		assert(event->type == SDL_MOUSEBUTTONDOWN);
		if (isOnWidget(event->button.x, event->button.y) &&
				  (event->button.button == SDL_BUTTON_LEFT))
		parent->onAction(this, BUTTON_PRESSED, returnCode, 0);
	}
	
	void Button::onSDLMouseButtonUp(SDL_Event *event)
	{
		assert(event->type == SDL_MOUSEBUTTONUP);
		if (isOnWidget(event->button.x, event->button.y) &&
				(event->button.button == SDL_BUTTON_LEFT))
			parent->onAction(this, BUTTON_RELEASED, returnCode, 0);
	}
	
	
	TextButton::TextButton(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, const char *sprite, int standardId, int highlightID, const char *font, const char *text, int returnCode, Uint16 unicode)
	:Button(x, y, w, h, hAlign, vAlign, sprite, standardId, highlightID, returnCode, unicode)
	{
		assert(font);
		assert(text);
		this->font=font;
		this->text=text;
		fontPtr=NULL;
	}

	TextButton::TextButton(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, const char *sprite, int standardId, int highlightID, const char *font, const char *text, int returnCode,
		const std::string& tooltip, const std::string &tooltipFont, Uint16 unicode)
		:Button(x, y, w, h, hAlign, vAlign, sprite, standardId, highlightID, returnCode, tooltip, tooltipFont, unicode)
	{
		assert(font);
		assert(text);
		this->font=font;
		this->text=text;
		fontPtr=NULL;
	}


	void TextButton::internalInit(void)
	{
		Button::internalInit();
		fontPtr = Toolkit::getFont(font.c_str());
		assert(fontPtr);
	}
	
	void TextButton::paint()
	{
		int x, y, w, h;
		getScreenPos(&x, &y, &w, &h);
		
		assert(parent);
		assert(parent->getSurface());
		
		Style::style->drawTextButtonBackground(parent->getSurface(), x, y, w, h, getNextHighlightValue());
		
		int decX=(w-fontPtr->getStringWidth(this->text.c_str()))>>1;
		int decY=(h-fontPtr->getStringHeight(this->text.c_str()))>>1;
	
		parent->getSurface()->drawString(x+decX, y+decY, fontPtr, text.c_str());
	}
	
	void TextButton::setText(const char *text)
	{
		assert(text);
		this->text=text;
	}
	
	
	OnOffButton::OnOffButton(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, bool startState, int returnCode)
	:HighlightableWidget(returnCode)
	{
		this->x=x;
		this->y=y;
		this->w=w;
		this->h=h;
		this->hAlignFlag=hAlign;
		this->vAlignFlag=vAlign;
	
		this->state=startState;
	}
	
	OnOffButton::OnOffButton(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, bool startState, int returnCode, const std::string &tooltip, const std::string &tooltipFont)
	:HighlightableWidget(tooltip, tooltipFont, returnCode)
	{
		this->x=x;
		this->y=y;
		this->w=w;
		this->h=h;
		this->hAlignFlag=hAlign;
		this->vAlignFlag=vAlign;
	
		this->state=startState;
	}
	
	void OnOffButton::onSDLMouseButtonDown(SDL_Event *event)
	{
		assert(event->type == SDL_MOUSEBUTTONDOWN);
		if (isOnWidget(event->button.x, event->button.y) &&
			(event->button.button == SDL_BUTTON_LEFT))
		{
			state=!state;
			parent->onAction(this, BUTTON_PRESSED, returnCode, 0);
			parent->onAction(this, BUTTON_STATE_CHANGED, returnCode, state == true ? 1 : 0);
		}
	}
	
	void OnOffButton::onSDLMouseButtonUp(SDL_Event *event)
	{
		assert(event->type == SDL_MOUSEBUTTONUP);
		if (isOnWidget(event->button.x, event->button.y))
				parent->onAction(this, BUTTON_RELEASED, returnCode, 0);
	}
	
	void OnOffButton::paint()
	{
		int x, y, w, h;
		getScreenPos(&x, &y, &w, &h);
		
		assert(parent);
		assert(parent->getSurface());
		
		Style::style->drawOnOffButton(parent->getSurface(), x, y, w, h, getNextHighlightValue(), state);
	}
	
	void OnOffButton::setState(bool newState)
	{
		if (newState!=state)
		{
			state=newState;
		}
	}
	
	ColorButton::ColorButton(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, int returnCode)
	:HighlightableWidget(returnCode)
	{
		this->x=x;
		this->y=y;
		this->w=w;
		this->h=h;
		this->hAlignFlag=hAlign;
		this->vAlignFlag=vAlign;
	
		selColor=0;
	}

	ColorButton::ColorButton(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, const std::string& tooltip, const std::string &tooltipFont, int returnCode)
	:HighlightableWidget(tooltip, tooltipFont, returnCode)
	{
		this->x=x;
		this->y=y;
		this->w=w;
		this->h=h;
		this->hAlignFlag=hAlign;
		this->vAlignFlag=vAlign;
	
		selColor=0;
	}
	
	void ColorButton::onMouseButtonDown(SDL_Event *event)
	{
		if (isOnWidget(event->button.x, event->button.y) && v.size())
		{
			if (event->button.button == SDL_BUTTON_LEFT)
			{
				selColor++;
				if (selColor>=(signed)v.size())
					selColor=0;
		
				parent->onAction(this, BUTTON_STATE_CHANGED, returnCode, selColor);
				parent->onAction(this, BUTTON_PRESSED, returnCode, 0);
			}
			else if (event->button.button == SDL_BUTTON_RIGHT)
			{
				selColor--;
				if (selColor<0)
					selColor=(signed)v.size()-1;
				
				parent->onAction(this, BUTTON_STATE_CHANGED, returnCode, selColor);
				parent->onAction(this, BUTTON_PRESSED, returnCode, 0);
			}
		}
	}
	
	void ColorButton::onMouseButtonUp(SDL_Event *event)
	{
		if (isOnWidget(event->button.x, event->button.y) &&
				(event->button.button == SDL_BUTTON_LEFT))
		{
			parent->onAction(this, BUTTON_RELEASED, returnCode, 0);
		}
	}
	
	void ColorButton::paint()
	{
		int x, y, w, h;
		getScreenPos(&x, &y, &w, &h);
		
		assert(parent);
		assert(parent->getSurface());
		
		if (v.size())
			parent->getSurface()->drawFilledRect(x+1, y+1, w-2, h-2, v[selColor].r, v[selColor].g, v[selColor].b);
		HighlightableWidget::paint();
	}
	
	MultiTextButton::MultiTextButton(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, const char *sprite, int standardId, int highlightID, const char *font, const char *text, int returnCode, Uint16 unicode)
	:TextButton(x, y, w, h, hAlign, vAlign, sprite, standardId, highlightID, font, text, returnCode, unicode)
	{
		textIndex = 0;
	}
	MultiTextButton::MultiTextButton(int x, int y, int w, int h, Uint32 hAlign, Uint32 vAlign, const char *sprite, int standardId, int highlightID, const char *font, const char *text, int returnCode, const std::string& tooltip, const std::string &tooltipFont, Uint16 unicode)
	:TextButton(x, y, w, h, hAlign, vAlign, sprite, standardId, highlightID, font, text, returnCode, tooltip, tooltipFont, unicode)
	{
		textIndex = 0;
	}
	
	void MultiTextButton::onSDLMouseButtonDown(SDL_Event *event)
	{
		assert(event->type == SDL_MOUSEBUTTONDOWN);
		if (isOnWidget(event->button.x, event->button.y) && texts.size())
		{
			if (event->button.button == SDL_BUTTON_LEFT)
			{
				textIndex++;
				if (textIndex >= texts.size())
					textIndex = 0;
				setText(texts.at(textIndex));
	
				parent->onAction(this, BUTTON_STATE_CHANGED, returnCode, textIndex);
				parent->onAction(this, BUTTON_PRESSED, returnCode, 0);
			}
			else if (event->button.button == SDL_BUTTON_RIGHT)
			{
				if (textIndex > 0)
					textIndex--;
				else
					textIndex = texts.size()-1;
				setText(texts.at(textIndex));
				
				parent->onAction(this, BUTTON_STATE_CHANGED, returnCode, textIndex);
				parent->onAction(this, BUTTON_PRESSED, returnCode, 0);
			}
		}
	}
	
	void MultiTextButton::onSDLMouseButtonUp(SDL_Event *event)
	{
		assert(event->type == SDL_MOUSEBUTTONUP);
		if (isOnWidget(event->button.x, event->button.y) &&
			(event->button.button == SDL_BUTTON_LEFT))
		{
			parent->onAction(this, BUTTON_RELEASED, returnCode, 0);
		}
	}
	
	void MultiTextButton::addText(const char *s)
	{
		texts.push_back(s);
	}
	
	void MultiTextButton::clearTexts(void)
	{
		texts.clear();
	}
	
	void MultiTextButton::setIndex(int i)
	{
		textIndex = i;
		setText(texts.at(textIndex));
	}
	
	void MultiTextButton::setIndexFromText(const std::string &s)
	{
		for (size_t i = 0; i < texts.size(); i++)
		{
			if (texts[i] == s)
				setIndex(i);
		}
	}
}
