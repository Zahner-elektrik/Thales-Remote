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

#include <thread>

#include "thalesremoteconnection.h"
#include "thalesremotescriptwrapper.h"


#define TARGET_HOST "192.168.2.78"

void spectrum(ThalesRemoteScriptWrapper &scriptHandle, double lower_frequency, double upper_frequency, int number_of_points);
void printImpedance(std::complex<double> impedance);

int main(int argc, char *argv[]) {

    ThalesRemoteConnection thalesConnection;

    bool connectionSuccessful = thalesConnection.connectToTerm(TARGET_HOST, "ScriptRemote");

    if (connectionSuccessful == false) {

        std::cout << "Could not connect to Term" << std::endl;
        return 0;
    }

    ThalesRemoteScriptWrapper remoteScript(&thalesConnection);

    remoteScript.forceThalesIntoRemoteScript();

    remoteScript.setPotentiostatMode(ThalesRemoteScriptWrapper::POTMODE_POTENTIOSTATIC);
    remoteScript.setPotential(0);
    remoteScript.enablePotentiostat();

    std::cout << "Potential " << remoteScript.getPotential() << std::endl;
    std::cout << "Current " << remoteScript.getCurrent()   << std::endl;

    remoteScript.setFrequency(2000);
    remoteScript.setAmplitude(10e-3);
    remoteScript.setNumberOfPeriods(3);

    printImpedance(remoteScript.getImpedance());
    printImpedance(remoteScript.getImpedance(2000));
    printImpedance(remoteScript.getImpedance(2000, 10e-3, 3));

    spectrum(remoteScript, 1000, 2e5, 10);

    thalesConnection.disconnectFromTerm();

    return 0;
}

void spectrum(ThalesRemoteScriptWrapper &scriptHandle, double lower_frequency, double upper_frequency, int number_of_points) {

    double log_lower_frequency = std::log(lower_frequency);
    double log_upper_frequency = std::log(upper_frequency);

    double log_interval_spacing = (log_upper_frequency - log_lower_frequency) / static_cast<double>(number_of_points - 1);

    for (int i = number_of_points - 1; i >= 0; --i) {

        // calculating logarithmic equidistant frequencies
        double current_frequency = std::exp(log_lower_frequency + log_interval_spacing * static_cast<double>(i));

        std::cout << "Frequency " << current_frequency << std::endl;
        printImpedance(scriptHandle.getImpedance(current_frequency));
    }
}

void printImpedance(std::complex<double> impedance) {

    std::cout << std::abs(impedance) << " ohm, " << std::arg(impedance) << " rad" << std::endl;
}
