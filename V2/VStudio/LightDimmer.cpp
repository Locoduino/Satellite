/*
 * Light Dimmer Library for Arduino
 *
 * Copyright Jean-Luc Béchennec 2018
 *
 * This software is distributed under the GNU Public Licence v2 (GPLv2)
 *
 * Please read the LICENCE file
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <LightDimmer.h>

LightDimmer *LightDimmer::sLightList = NULL;

LightDimmer::LightDimmer()
  : mState(LD_OFF),
    mMax(255u),
    mRiseTime(250u),
    mFallTime(250u),
    mOnTime(200u),
    mPeriod(900u),
    mNextEventDate(0u),
    mValue(0u),
    mPin(63),
    mBlink(false),
    mOff(LOW)
{
  mNext = sLightList;
  sLightList = this;
}

void LightDimmer::begin(const uint8_t inPin, const uint8_t inOn)
{
  mPin = inPin;
  mOff = !inOn;
  pinMode(mPin, OUTPUT);
  /* At start, le LED is switched off */
  digitalWrite(mPin, mOff);
}

void LightDimmer::on()
{
  switch (mState)
  {
    case LD_OFF:
      mNextEventDate = millis() + mRiseTime;
      mState = LD_RISING;
      break;
    case LD_FALLING:
      uint32_t remainingTime = mNextEventDate - millis();
      mNextEventDate = millis() + (uint32_t)mRiseTime * remainingTime / (uint32_t)mFallTime;
      mState = LD_RISING;
      break;
  }
  mBlink = false;
}

void LightDimmer::off()
{
  switch (mState)
  {
    case LD_ON:
      mNextEventDate = millis() + mFallTime;
      mState = LD_FALLING;
      break;
    case LD_RISING:
      uint32_t remainingTime = mNextEventDate - millis();
      mNextEventDate = millis() + (uint32_t)mFallTime * remainingTime / (uint32_t)mRiseTime;
      mState = LD_FALLING;
      break;
  }
  mBlink = false;
}

void LightDimmer::startBlink()
{
  if (mPeriod >= mRiseTime + mOnTime + mFallTime) {
    mBlink = true;
  }
}

void LightDimmer::updateState()
{
  uint32_t currentDate = millis();
  switch (mState) {
    case LD_OFF:
      if (mBlink) {
        if (currentDate >= mNextEventDate) {
          mNextEventDate = millis() + mRiseTime;
          mState = LD_RISING;
        }
      }
      break;
    case LD_RISING:
      if (currentDate < mNextEventDate) {
        uint8_t value = mMax * (currentDate - (mNextEventDate - mRiseTime)) / mRiseTime;
        mValue = mOff ? 255 - value : value;
      }
      else {
        mValue = mOff ? 255 - mMax : mMax ;
        mNextEventDate = currentDate + mPeriod - mOnTime - mRiseTime -mFallTime;
        mState = LD_ON;
      }
      break;
    case LD_ON:
      if (mBlink) {
        if (currentDate >= mNextEventDate) {
          mNextEventDate = currentDate + mFallTime;
          mState = LD_FALLING;
        }
      }
      break;
    case LD_FALLING:
      if (currentDate < mNextEventDate) {
        uint8_t value = mMax * (currentDate - (mNextEventDate - mFallTime)) / mFallTime;
        mValue = mOff ? value + 255 - mMax : mMax - value;
      }
      else {
        mValue = mOff ? 255 : 0;
        mState = LD_OFF;
        mNextEventDate = currentDate + mPeriod - mOnTime - mRiseTime - mFallTime;
      }
      break;
  }
  updateOutput();
}

void LightDimmer::updateOutput()
{
  analogWrite(mPin, mValue);
}

void LightDimmer::update()
{
  LightDimmer *ld = sLightList;
  while (ld != NULL) {
    ld->updateState();
    ld = ld->mNext;
  }
}

void LightDimmerSoft::updateOutput()
{
  mDuty += mOff + (mValue >> 3);       /* Add the value to the duty on 5 bits */
  uint8_t out = ((mDuty & 0xE0) != 0); /* The output is the carry             */
  digitalWrite(mPin, out);             /* Set the OUTPUT                      */
  mDuty &= 0x1F;                       /* Clear the carry                     */
}
