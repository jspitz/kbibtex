/***************************************************************************
*   Copyright (C) 2004-2012 by Thomas Fischer                             *
*   fischer@unix-ag.uni-kl.de                                             *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/
#ifndef KBIBTEX_IO_ENCODERUTF8_H
#define KBIBTEX_IO_ENCODERUTF8_H

#include "kbibtexio_export.h"

#include "encoderlatex.h"

class QString;
class QRegExp;

/**
 * Base class for that convert between different textual representations
 * for non-ASCII characters, specialized for UTF-8.
 * Example for a character to convert is &auml;.
 * @author Thomas Fischer <fischer@unix-ag.uni-kl.de>
 */
class KBIBTEXIO_EXPORT EncoderUTF8 : public EncoderLaTeX
{
public:
    virtual QString encode(const QString &text) const;

    static EncoderUTF8 *instance();

private:
    EncoderUTF8();
    ~EncoderUTF8();
    static EncoderUTF8 *self;
};

#endif // KBIBTEX_IO_ENCODERUTF8_H