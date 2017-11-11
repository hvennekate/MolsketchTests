/***************************************************************************
 *   Copyright (C) 2017 by Hendrik Vennekate                               *
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
#include "xmlassertion.h"
#include <execinfo.h>
#include <stacktrace.hpp>
#include <iostream>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QTextStream>
#include <QXmlStreamReader>

const int MAX_STACK_TRACE_DEPTH = 15;

class XmlAssertionPrivate {
  Q_DISABLE_COPY(XmlAssertionPrivate)
public:
  XmlAssertionPrivate() {}
  QXmlQuery query;
  QString xml, queryString;
  void printStackTraceAndThrow(const QString &message);
  static QString formatXml(const QString &xml);
};

XmlAssertion *XmlAssertion::assertThat(const QString &xml) {
  return new XmlAssertion(xml);
}

XmlAssertion *XmlAssertion::assertThat(const Molsketch::XmlObjectInterface &object) {
  QString xml;
  QXmlStreamWriter out(&xml);
  object.writeXml(out);
  return new XmlAssertion(xml);
}

QString XmlAssertion::formatXml(const QString &xml) {
  return XmlAssertionPrivate::formatXml(xml);
}

QString XmlAssertionPrivate::formatXml(const QString &xml) {
  QXmlStreamReader reader(xml);
  QString formattedXml;
  QXmlStreamWriter writer(&formattedXml);
  writer.setAutoFormatting(true);

  while (!reader.atEnd()) {
      reader.readNext();
      if (!reader.isWhitespace()) {
          writer.writeCurrentToken(reader);
      }
  }
  return formattedXml;
}

void XmlAssertionPrivate::printStackTraceAndThrow(const QString& message) {
  QTextStream out(stdout, QIODevice::WriteOnly);
  out << message << endl;
  out << "Query:" << endl << queryString << endl
           << "XML:" << endl << formatXml(xml) << endl;
  // TODO parent matches etc.
#ifdef MSKTEST_STACKTRACE
  std::cout << boost::stacktrace::stacktrace();
#endif
  throw message;
}

XmlAssertion *XmlAssertion::contains(const QString &xQuery) {
  Q_D(XmlAssertion);
  d->queryString = xQuery;
  d->query.setQuery(xQuery);
  return this;
}

XmlAssertion *XmlAssertion::exactlyOnceWithContent(const QString &expected) {
  Q_D(XmlAssertion);
  QXmlResultItems results;
  d->query.evaluateTo(&results);
  if (results.hasError())
    d->printStackTraceAndThrow("Error in query!");
  QXmlItem item = results.next();
  if (!item.isAtomicValue())
    d->printStackTraceAndThrow("Expected item from query to be an atomic value.");
  QString itemValue(item.toAtomicValue().toString());
  if (itemValue != expected)
    d->printStackTraceAndThrow("Item value does not match.  Expected: \"" + expected + "\" Actual: \"" + itemValue + "\"");
  if (!results.next().isNull())
    d->printStackTraceAndThrow("More than one match to query found!");
  return this;
}

XmlAssertion *XmlAssertion::never() {
  Q_D(XmlAssertion);
  QXmlResultItems results;
  d->query.evaluateTo(&results);
  if (results.hasError())
    d->printStackTraceAndThrow("Error in query!");
  if (!results.next().isNull())
    d->printStackTraceAndThrow("No match for query expected!");
  return this;
}

XmlAssertion::XmlAssertion(const QString &xml)
  : d_ptr(new XmlAssertionPrivate)
{
  Q_D(XmlAssertion);
  d->xml = xml;
  d->query.setFocus(xml);
}

XmlAssertion *XmlAssert::assertThat(const QString &xml) { return XmlAssertion::assertThat(xml); }

XmlAssertion *XmlAssert::assertThat(const Molsketch::XmlObjectInterface &object) {
  return XmlAssertion::assertThat(object);
}