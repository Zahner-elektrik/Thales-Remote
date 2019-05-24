/******************************************************************
 *  ____       __                        __    __   __      _ __
 * /_  / ___ _/ /  ___  ___ ___________ / /__ / /__/ /_____(_) /__
 *  / /_/ _ `/ _ \/ _ \/ -_) __/___/ -_) / -_)  '_/ __/ __/ /  '_/
 * /___/\_,_/_//_/_//_/\__/_/      \__/_/\__/_/\_\\__/_/ /_/_/\_\
 *
 * Copyright 2019 ZAHNER-elektrik I. Zahner-Schiller GmbH & Co. KG
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
 * THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "thalesremotescriptwrapper.h"

ThalesRemoteScriptWrapper::ThalesRemoteScriptWrapper(ThalesRemoteConnection * const remoteConnection) :
    remoteConnection(remoteConnection)
{

}

std::string ThalesRemoteScriptWrapper::executeRemoteCommand(std::string command) {
    return remoteConnection->sendAndWaitForReply("1:" + command + ":", 2);
}

void ThalesRemoteScriptWrapper::forceThalesIntoRemoteScript() {

    remoteConnection->sendAndWaitForReply("<\x02>,ScriptRemote", 0x80);
}

double ThalesRemoteScriptWrapper::getCurrent()
{

    return this->requestValueAndParseUsingRegexp("CURRENT", std::regex("current=\\s*(.*?)A?:"));
}

double ThalesRemoteScriptWrapper::getPotential() {

    return this->requestValueAndParseUsingRegexp("POTENTIAL", std::regex("potential=\\s*(.*?)V?:"));
}

void ThalesRemoteScriptWrapper::setCurrent(double current) {

    this->executeRemoteCommand("Cset=" + std::to_string(current));
}

void ThalesRemoteScriptWrapper::setPotential(double potential) {

    this->executeRemoteCommand("Pset=" + std::to_string(potential));
}

void ThalesRemoteScriptWrapper::enablePotentiostat(bool enabled) {

    if (enabled == true) {

        this->executeRemoteCommand("Pot=-1");
    } else {

        this->executeRemoteCommand("Pot=0");
    }
}

void ThalesRemoteScriptWrapper::setPotentiostatMode(ThalesRemoteScriptWrapper::PotentiostatMode potentiostatMode)
{
    std::string command;

    switch(potentiostatMode) {

    case POTMODE_POTENTIOSTATIC:

        command = "Gal=0:GAL=0";
        break;

    case POTMODE_GALVANOSTATIC:

        command = "Gal=-1:GAL=1";
        break;

    case POTMODE_PSEUDOGALVANOSTATIC:

        command = "Gal=0:GAL=-1";
        break;

    default:
        return;
        break;
    }

    this->executeRemoteCommand(command);
}

void ThalesRemoteScriptWrapper::setFrequency(double frequency) {

    this->executeRemoteCommand("Frq=" + std::to_string(frequency));
}

void ThalesRemoteScriptWrapper::setAmplitude(double amplitude) {

    this->executeRemoteCommand("Ampl=" + std::to_string(amplitude * 1e3));
}

void ThalesRemoteScriptWrapper::setNumberOfPeriods(int number_of_periods) {

    // little bits of stability

    if (number_of_periods > 100) {
        number_of_periods = 100;
    }

    if (number_of_periods < 1) {
        number_of_periods = 1;
    }

    this->executeRemoteCommand("Nw=" + std::to_string(number_of_periods));
}

std::complex<double> ThalesRemoteScriptWrapper::getImpedance() {

    std::complex<double> result(std::nan("1"), std::nan("1"));

    std::string reply = this->executeRemoteCommand("IMPEDANCE");

    std::regex replyStringPattern("impedance=\\s*(.*?),(.*?):");
    std::smatch match;

    std::regex_search(reply, match, replyStringPattern);

    if (match.size() > 2) {

        result = std::complex<double>(std::stod(match.str(1)), std::stod(match.str(2)));
    }

    return result;
}

std::complex<double> ThalesRemoteScriptWrapper::getImpedance(double frequency) {

    this->setFrequency(frequency);

    return this->getImpedance();
}

std::complex<double> ThalesRemoteScriptWrapper::getImpedance(double frequency, double amplitude, int number_of_periods) {

    this->setFrequency(frequency);
    this->setAmplitude(amplitude);
    this->setNumberOfPeriods(number_of_periods);

    return this->getImpedance();
}

double ThalesRemoteScriptWrapper::requestValueAndParseUsingRegexp(std::string command, std::regex pattern)
{
    double result = std::nan("1");

    std::string reply = this->executeRemoteCommand(command);

    std::regex replyStringPattern(pattern);
    std::smatch match;

    std::regex_search(reply, match, replyStringPattern);

    if (match.size() > 1) {

        result = std::stod(match.str(1));
    }

    return result;
}
