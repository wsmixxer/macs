/*******************************************************
*    These are the license terms for Button.cpp & Button.h

*    Circuito.io is an automatic generator of schematics and code for off
*    the shelf hardware combinations.

*    Copyright (C) 2016 Roboplan Technologies Ltd.

*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.

*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*    In addition, and without limitation, to the disclaimers of warranties
*    stated above and in the GNU General Public License version 3 (or any
*    later version), Roboplan Technologies Ltd. ("Roboplan") offers this
*    program subject to the following warranty disclaimers and by using
*    this program you acknowledge and agree to the following:
*    THIS PROGRAM IS PROVIDED ON AN "AS IS" AND "AS AVAILABLE" BASIS, AND
*    WITHOUT WARRANTIES OF ANY KIND EITHER EXPRESS OR IMPLIED.  ROBOPLAN
*    HEREBY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING BUT
*    NOT LIMITED TO IMPLIED WARRANTIES OF MERCHANTABILITY, TITLE, FITNESS
*    FOR A PARTICULAR PURPOSE, NON-INFRINGEMENT, AND THOSE ARISING BY
*    STATUTE OR FROM A COURSE OF DEALING OR USAGE OF TRADE.
*    YOUR RELIANCE ON, OR USE OF THIS PROGRAM IS AT YOUR SOLE RISK.
*    ROBOPLAN DOES NOT GUARANTEE THAT THE PROGRAM WILL BE FREE OF, OR NOT
*    SUSCEPTIBLE TO, BUGS, SECURITY BREACHES, OR VIRUSES. ROBOPLAN DOES
*    NOT WARRANT THAT YOUR USE OF THE PROGRAM, INCLUDING PURSUANT TO
*    SCHEMATICS, INSTRUCTIONS OR RECOMMENDATIONS OF ROBOPLAN, WILL BE SAFE
*    FOR PERSONAL USE OR FOR PRODUCTION OR COMMERCIAL USE, WILL NOT
*    VIOLATE ANY THIRD PARTY RIGHTS, WILL PROVIDE THE INTENDED OR DESIRED
*    RESULTS, OR OPERATE AS YOU INTENDED OR AS MAY BE INDICATED BY ROBOPLAN.
*    YOU HEREBY WAIVE, AGREE NOT TO ASSERT AGAINST, AND RELEASE ROBOPLAN,
*    ITS LICENSORS AND AFFILIATES FROM, ANY CLAIMS IN CONNECTION WITH ANY OF
*    THE ABOVE.
********************************************************/

#include "Button.h"

#include <Particle.h>


Button::Button(const int pin) : m_pin(pin)
{

}

void Button::init()
{

  pinMode(m_pin, INPUT);
  // set begin state
  m_lastButtonState = read();
}


//read button state.
bool Button::read()
{
  return digitalRead(m_pin);

}


//In General:
//if button is not pushed function will return LOW (0).
//if it is pushed function will return HIGH (1).

bool Button::onChange()
{
  //read button state. '1' is pushed, '0' is not pushed.
  bool reading = read();
  // If the switch changed, due to noise or pressing:
  if (reading != m_lastButtonState) {
    // reset the debouncing timer
    m_lastDebounceTime = millis();
    m_pressFlag = 1;
  }

  if ((millis() - m_lastDebounceTime) > m_debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (m_pressFlag) { //reading != m_lastButtonState) {
      //update the buton state
      m_pressFlag = 0;

      // save the reading.  Next time through the loop,
      m_lastButtonState = reading;
      return 1;

    }
  }
  m_lastButtonState = reading;

  return 0;

}

bool Button::onPress()
{
  //read button state. '1' is pushed, '0' is not pushed.
  bool reading = read();
  // If the switch changed, due to noise or pressing:
  if (reading == HIGH && m_lastButtonState == LOW) {
    // reset the debouncing timer
    m_lastDebounceTime = millis();
    m_pressFlag = 1;
  }

  if ((millis() - m_lastDebounceTime) > m_debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (m_pressFlag) {
      // save the reading.  Next time through the loop,
      m_pressFlag = 0;
      m_lastButtonState = reading;
      return 1;

    }
  }
  m_lastButtonState = reading;

  return 0;

}

bool Button::onRelease()
{
  //read button state. '1' is pushed, '0' is not pushed.
  bool reading = read();
  // If the switch changed, due to noise or pressing:
  if (reading == LOW && m_lastButtonState == HIGH) {
    // reset the debouncing timer
    m_lastDebounceTime = millis();
    m_pressFlag = 1;
  }

  if ((millis() - m_lastDebounceTime) > m_debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if ( m_pressFlag) { //reading == LOW && m_lastButtonState == HIGH) {
      // save the reading.  Next time through the loop,
      m_lastButtonState = reading;
      m_pressFlag = 0;
      return 1;

    }
  }
  m_lastButtonState = reading;

  return 0;

}
