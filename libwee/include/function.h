/*
  Copyright (C) 2004 Martin Voelkle <martin.voelkle@epfl.ch>

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

#ifndef FUNCTION_H
#define FUNCTION_H

#include "container.h"

struct Function {
	virtual void call(Value* frame) = 0;
};

namespace Functions {
	
	struct Native {
		void call(Stack* stack) { code(stack); }
		void (*code)(Stack*);
	};
	
	struct Interpreted {
		void call(Stack* stack);
		uint8_t* code;
	};
	
};

#endif // ndef FUNCTION_H