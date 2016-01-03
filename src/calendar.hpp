#pragma once

#include "vec.hpp"

class h_Calendar
{
public:
	/*!
	 * \brief h_Calendar
	 * \param ticks_in_day
	 * \param solar_days_in_year
	 * \param rotation_axis_angle - angle between orbital plane and rotation axis
	 * \param summer_solstice_day - longest day in northen hemisphere
	 */
	h_Calendar(
		unsigned int ticks_in_day,
		unsigned int solar_days_in_year,
		float rotation_axis_angle,
		unsigned int summer_solstice_day );

	h_Calendar(const h_Calendar&)= delete;
	h_Calendar& operator=(const h_Calendar&)= delete;

	unsigned int GetTicksInDay() const;
	unsigned int GetDaysInYear() const;
	const m_Vec3& GetRotationAxis() const;
	// Out - in ticks
	unsigned int GetNightLength( unsigned int day, float latitude ) const;

	float GetSkySphereRotation( unsigned int ticks ) const;
	// Get local sun vector direction
	m_Vec3 GetSunVector( unsigned int ticks, float latitude ) const;

private:
	m_Vec3 GetVectorFromPlanetToStar( unsigned int ticks ) const;

private:
	unsigned int ticks_in_day_;
	unsigned int solar_days_in_year_;

	float rotation_axis_angle_;
	m_Vec3 rotation_axis_;
	unsigned int summer_solstice_day_;
};
