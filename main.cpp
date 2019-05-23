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

        std::cout << "frequency " << current_frequency << std::endl;
        printImpedance(scriptHandle.getImpedance(current_frequency));
    }
}

void printImpedance(std::complex<double> impedance) {

    std::cout << std::abs(impedance) << "ohm, " << std::arg(impedance) << "rad" << std::endl;
}
