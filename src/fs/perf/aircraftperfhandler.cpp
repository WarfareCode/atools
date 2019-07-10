/*****************************************************************************
* Copyright 2015-2019 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#include "fs/perf/aircraftperfhandler.h"

#include "fs/perf/aircraftperf.h"
#include "fs/sc/simconnectdata.h"
#include "atools.h"
#include "geo/calculations.h"
#include "fs/sc/simconnectuseraircraft.h"

namespace atools {
namespace fs {
namespace perf {

using atools::fs::sc::SimConnectUserAircraft;
using atools::fs::sc::SimConnectData;
using atools::roundToInt;
using atools::fs::perf::AircraftPerf;

AircraftPerfHandler::AircraftPerfHandler(QObject *parent)
  : QObject(parent)
{
  perf = new AircraftPerf;
  perf->setNull();
}

AircraftPerfHandler::~AircraftPerfHandler()
{
  delete perf;
}

void AircraftPerfHandler::start()
{
  currentFlightSegment = NONE;
  startFuel = totalFuelConsumed = weightVolRatio = 0.f;

  lastSampleTimeMs = lastCruiseSampleTimeMs = lastClimbSampleTimeMs = lastDescentSampleTimeMs = 0L;

  aircraftClimb = aircraftDescent = aircraftFuelFlow = aircraftGround = aircraftFlying = false;
  aircraftCruise = 0;

  perf->setNull();

  active = true;
}

void AircraftPerfHandler::reset()
{
  start();
}

void AircraftPerfHandler::stop()
{
  active = false;
}

void AircraftPerfHandler::simDataChanged(const sc::SimConnectData& simulatorData)
{
  const sc::SimConnectUserAircraft& aircraft = simulatorData.getUserAircraftConst();
  if(!active || !aircraft.isValid() || aircraft.isSimPaused() || aircraft.isSimReplay())
    return;

  aircraftClimb = isClimbing(aircraft);
  aircraftDescent = isDescending(aircraft);
  aircraftCruise = isAtCruise(aircraft);
  aircraftFuelFlow = aircraft.hasFuelFlow();
  aircraftGround = aircraft.isOnGround();
  aircraftFlying = aircraft.isFlying();

  // Fill metadata if still empty
  if(perf->getAircraftType().isEmpty())
    perf->setAircraftType(aircraft.getAirplaneModel());

  if(perf->getName().isEmpty())
    perf->setName(aircraft.getAirplaneTitle());

  // Determine fuel type ========================================================
  if(atools::almostEqual(weightVolRatio, 0.f))
  {
    bool jetfuel = atools::geo::isJetFuel(aircraft.getFuelTotalWeightLbs(),
                                          aircraft.getFuelTotalQuantityGallons(), weightVolRatio);

    if(weightVolRatio > 0.f)
    {
      perf->setJetFuel(jetfuel);
      qDebug() << Q_FUNC_INFO << "weightVolRatio" << weightVolRatio << "jetfuel" << perf->isJetFuel();
    }
    // else insufficient fuel amount
  }

  // Remember fuel in tanks if not done already ========================================================
  // Delay fuel calculation until there is fuel flow to avoid catching user changes
  // in fuel amount before flight
  if(startFuel < 0.1f && aircraftFuelFlow)
  {
    startFuel = aircraft.getFuelTotalWeightLbs();
    qDebug() << Q_FUNC_INFO << "startFuel" << startFuel;
  }

  if(aircraftFuelFlow)
    totalFuelConsumed = startFuel - aircraft.getFuelTotalWeightLbs();

  // Determine current flight sement ================================================================
  FlightSegment flightSegment = currentFlightSegment;
  if(aircraft.isValid())
  {
    switch(currentFlightSegment)
    {
      case INVALID:
        break;

      case NONE:
        // Nothing sampled yet - start from scratch ==============
        if(aircraftGround)
          flightSegment = aircraftFuelFlow ? DEPARTURE_TAXI : DEPARTURE_PARKING;
        else if(aircraftCruise == 0)
          flightSegment = CRUISE;
        else if(isClimbing(aircraft) && aircraftCruise == -1)
          flightSegment = CLIMB;
        else if(isDescending(aircraft) && aircraftCruise == -1)
          flightSegment = DESCENT;
        break;

      case DEPARTURE_PARKING:
        if(aircraftFuelFlow)
          flightSegment = DEPARTURE_TAXI;
        if(aircraftFlying)
          // Skip directly to climb if in the air
          flightSegment = CLIMB;
        break;

      case DEPARTURE_TAXI:
        if(aircraftFlying)
          flightSegment = CLIMB;
        break;

      case CLIMB:
        if(aircraftCruise >= 0)
          // At cruise - 200 ft or above
          flightSegment = CRUISE;
        break;

      case CRUISE:
        if(aircraftCruise < 0)
          // Below cruise - start descent
          flightSegment = DESCENT;
        break;

      case DESCENT:
        if(!aircraftFlying)
          // Landed
          flightSegment = DESTINATION_TAXI;

        if(aircraftCruise >= 0)
          // Momentary deviation  go back to cruise
          flightSegment = CRUISE;
        break;

      case DESTINATION_TAXI:
        if(!aircraftFuelFlow)
          // Engine shutdown
          flightSegment = DESTINATION_PARKING;
        break;

      case DESTINATION_PARKING:
        // Finish on engine shutdown - stop collecting
        active = false;
        break;
    }
  }

  // Remember segment dependent sample time to allow averaging =============
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  if(flightSegment != currentFlightSegment)
  {
    if(flightSegment == CLIMB)
      lastClimbSampleTimeMs = now;
    else if(flightSegment == CRUISE)
      lastCruiseSampleTimeMs = now;
    else if(flightSegment == DESCENT)
      lastDescentSampleTimeMs = now;
  }

  // Sum up taxi fuel  ========================================================
  if(currentFlightSegment == DEPARTURE_TAXI && aircraftFuelFlow)
    perf->setTaxiFuel(startFuel - aircraft.getFuelTotalWeightLbs());

  // Sample every 500 ms ========================================
  if(now > lastSampleTimeMs + SAMPLE_TIME_MS)
  {
    samplePhase(flightSegment, aircraft, now, now - lastSampleTimeMs);
    lastSampleTimeMs = now;
  }

  // Send message is flight segment has changed  ========================
  if(flightSegment != currentFlightSegment)
  {
    currentFlightSegment = flightSegment;
    emit flightSegmentChanged(currentFlightSegment);
  }
}

bool AircraftPerfHandler::isFinished() const
{
  return currentFlightSegment == DESTINATION_TAXI || currentFlightSegment == DESTINATION_PARKING;
}

void AircraftPerfHandler::setAircraftPerformance(const AircraftPerf& value)
{
  *perf = value;
}

QStringList AircraftPerfHandler::getAircraftStatusTexts()
{
  QStringList retval;
  if(aircraftGround)
  {
    retval.append(tr("on ground"));
    if(aircraftFuelFlow)
      retval.append(tr("fuel flow"));
  }

  if(aircraftFlying)
  {
    // retval.append(tr("flight"));
    if(aircraftClimb)
      retval.append(tr("climbing"));
    else if(aircraftDescent)
      retval.append(tr("descending"));
    else
    {
      if(aircraftCruise == 0)
        retval.append(tr("at cruise altitude"));
      else if(aircraftCruise < 0)
        retval.append(tr("below cruise altitude"));
      else if(aircraftCruise > 0)
        retval.append(tr("above cruise altitude"));
    }
  }

  if(!retval.isEmpty())
  {
    QString& first = retval.first();
    if(!first.isEmpty())
    {
      QChar firstChar = first.at(0);
      first.remove(0, 1);
      first.prepend(firstChar.toUpper());
    }
  }
  return retval;
}

float AircraftPerfHandler::sampleValue(qint64 lastSampleDuration, qint64 curSampleDuration, float lastValue,
                                       float curValue)
{
  if(lastSampleDuration == 0 || curSampleDuration == 0)
    return lastValue;

  // Calculate weighted average
  return static_cast<float>((lastValue * static_cast<double>(lastSampleDuration) +
                             curValue * static_cast<double>(curSampleDuration)) /
                            static_cast<double>(lastSampleDuration + curSampleDuration));
}

void AircraftPerfHandler::samplePhase(FlightSegment flightSegment, const SimConnectUserAircraft& aircraft,
                                      qint64 now, qint64 curSampleDuration)
{
  // Calculate all average values for each flight phase
  switch(flightSegment)
  {
    case NONE:
    case DEPARTURE_PARKING:
    case INVALID:
    case DESTINATION_PARKING:
    case DESTINATION_TAXI:
    case DEPARTURE_TAXI:
      break;

    case atools::fs::perf::CLIMB:
      {
        qint64 lastSampleDuration = now - lastClimbSampleTimeMs;
        perf->setClimbSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbSpeed(),
                                        aircraft.getTrueAirspeedKts()));
        perf->setClimbVertSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbVertSpeed(),
                                            aircraft.getVerticalSpeedFeetPerMin()));
        perf->setClimbFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getClimbFuelFlow(),
                                           aircraft.getFuelFlowPPH()));
      }
      break;

    case atools::fs::perf::CRUISE:
      {
        qint64 lastSampleDuration = now - lastCruiseSampleTimeMs;
        perf->setCruiseSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getCruiseSpeed(),
                                         aircraft.getTrueAirspeedKts()));
        perf->setCruiseFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getCruiseFuelFlow(),
                                            aircraft.getFuelFlowPPH()));

        // Use cruise as default for alternate - user can adjust manually
        perf->setAlternateFuelFlow(perf->getCruiseFuelFlow());
        perf->setAlternateSpeed(perf->getCruiseSpeed());
      }
      break;

    case atools::fs::perf::DESCENT:
      {
        qint64 lastSampleDuration = now - lastDescentSampleTimeMs;
        perf->setDescentSpeed(sampleValue(lastSampleDuration, curSampleDuration, perf->getDescentSpeed(),
                                          aircraft.getTrueAirspeedKts()));
        perf->setDescentVertSpeed(sampleValue(lastSampleDuration, curSampleDuration,
                                              perf->getDescentVertSpeed(),
                                              std::abs(aircraft.getVerticalSpeedFeetPerMin())));
        perf->setDescentFuelFlow(sampleValue(lastSampleDuration, curSampleDuration, perf->getDescentFuelFlow(),
                                             aircraft.getFuelFlowPPH()));
      }
      break;
  }
}

bool AircraftPerfHandler::isClimbing(const SimConnectUserAircraft& aircraft) const
{
  return aircraft.getVerticalSpeedFeetPerMin() > 150.f;
}

bool AircraftPerfHandler::isDescending(const SimConnectUserAircraft& aircraft) const
{
  return aircraft.getVerticalSpeedFeetPerMin() < -150.f;
}

int AircraftPerfHandler::isAtCruise(const SimConnectUserAircraft& aircraft) const
{
  float buffer = std::max(cruiseAltitude * 0.01f, 200.f);
  int result = !(aircraft.getIndicatedAltitudeFt() > cruiseAltitude - buffer &&
                 aircraft.getIndicatedAltitudeFt() < cruiseAltitude + buffer);

  if(result == 1)
  {
    // Use a larger buffer for deviations
    float buffer2 = std::max(cruiseAltitude * 0.02f, 200.f);
    if(aircraft.getIndicatedAltitudeFt() < cruiseAltitude - buffer2)
      result = -1;

    if(aircraft.getIndicatedAltitudeFt() > cruiseAltitude + buffer2)
      result = 1;
  }
  return result;
}

QString AircraftPerfHandler::getCurrentFlightSegmentString() const
{
  return getFlightSegmentString(currentFlightSegment);
}

QString AircraftPerfHandler::getFlightSegmentString(atools::fs::perf::FlightSegment currentFlightSegment)
{
  switch(currentFlightSegment)
  {
    case atools::fs::perf::INVALID:
      return tr("Invalid");

    case NONE:
      return tr("None");

    case DEPARTURE_PARKING:
      return tr("Departure Parking");

    case DEPARTURE_TAXI:
      return tr("Departure Taxi and Takeoff");

    case CLIMB:
      return tr("Climb");

    case CRUISE:
      return tr("Cruise");

    case DESCENT:
      return tr("Descent");

    case DESTINATION_TAXI:
      return tr("Destination Taxi");

    case DESTINATION_PARKING:
      return tr("Destination Parking");

  }
  return tr("Unknown");
}

} // namespace perf
} // namespace fs
} // namespace atools
