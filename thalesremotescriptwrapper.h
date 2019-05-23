#ifndef THALESREMOTESCRIPTWRAPPER_H
#define THALESREMOTESCRIPTWRAPPER_H

#include <regex>
#include <complex>

#include "thalesremoteconnection.h"

class ThalesRemoteScriptWrapper
{
public:

    enum PotentiostatMode {
        POTMODE_POTENTIOSTATIC,
        POTMODE_GALVANOSTATIC,
        POTMODE_PSEUDOGALVANOSTATIC
    };

    /** Constructor. Needs a connected ThalesRemoteConnection */
    ThalesRemoteScriptWrapper(ThalesRemoteConnection * const remoteConnection);

    /** Directly execute a query to Remote Script.
     *
     * \param [in] command The query string, e.g. "IMPEDANCE" or "Pset=0"
     *
     * \returns the reply sent by Remote Script
     */
    std::string executeRemoteCommand(std::string command);

    /** Prompts Thales to start the Remote Script
     *
     * Will switch a running Thales from anywhere like the main menu after
     * startup to running measurements into Remote Script so it can process
     * further requests.
     *
     * \warning This happens rather quickly if Thales is idle in main menu but
     *          may take some time if it's perofming an EIS measurement or doing
     *          something else. For high stability applications 20 seconds would
     *          probably be a save bet.
     */
    void forceThalesIntoRemoteScript();

    double getCurrent();
    double getPotential();

    void setCurrent(double current);
    void setPotential(double potential);

    /** Switch the potentiostat on or off.
     *
     * \param ]in] enabled switches the potentiostat on if true and off if false.
     */
    void enablePotentiostat(bool enabled = true);
    void setPotentiostatMode(PotentiostatMode potentiostatMode);

    void setFrequency(double frequency);
    void setAmplitude(double amplitude);

    /** Sets the number of periods to average for one impedance measurement.
     *
     * \param [in] number_of_periods the number of periods / waves to average.
     */
    void setNumberOfPeriods(int number_of_periods);

    /** Measure the impedance at the set frequency, amplitude and averages.
     *
     * \returns the complex impedance at the measured point.
     */
    std::complex<double> getImpedance();

    /** Measure the impedance at the set amplitude with set averages.
     *
     * \param [in] frequency the frequency to measure the impedance at.
     *
     * \returns the complex impedance at the measured point.
     */
    std::complex<double> getImpedance(double frequency);

    /** Measure the impedace with all possible parameters
     *
     * \param [in] frequency the frequency to measure the impedance at.
     * \param [in] amplitude the amplitude to measure the impedance with. In Volt if potentiostatic mode or Ampere for galvanostatic mode.
     * \param [in] number_of_periods the number of periods / waves to average.
     *
     * \returns the complex impedance at the measured point.
     */
    std::complex<double> getImpedance(double frequency, double amplitude, int number_of_periods = 1);

protected:

    double requestValueAndParseUsingRegexp(std::string command, std::regex pattern);

    ThalesRemoteConnection * const remoteConnection;
};

#endif // THALESREMOTESCRIPTWRAPPER_H
