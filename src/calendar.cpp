#include <cmath>

#include "math_lib/m_math.h"
#include "math_lib/assert.hpp"

#include "matrix.hpp"

#include "calendar.hpp"

#include "console.hpp"

h_Calendar::h_Calendar(
	unsigned int ticks_in_day,
	unsigned int solar_days_in_year,
	float rotation_axis_angle,
	unsigned int summer_solstice_day )
	: ticks_in_day_(ticks_in_day)
	, solar_days_in_year_(solar_days_in_year)
	, rotation_axis_angle_(rotation_axis_angle)
	, summer_solstice_day_(summer_solstice_day)
{
	float angle_to_summer_solstice_day= m_Math::FM_2PI * float(summer_solstice_day_) / float(solar_days_in_year_);

	rotation_axis_.x= -std::sin(rotation_axis_angle_) * std::cos(angle_to_summer_solstice_day);
	rotation_axis_.y= -std::sin(rotation_axis_angle_) * std::sin(angle_to_summer_solstice_day);
	rotation_axis_.z= +std::cos(rotation_axis_angle_);
}

unsigned int h_Calendar::GetTicksInDay() const
{
	return ticks_in_day_;
}

unsigned int h_Calendar::GetDaysInYear() const
{
	return solar_days_in_year_;
}

// Maybe, unfinished.
// Can contain bugs.
unsigned int h_Calendar::GetNightLength( unsigned int day, float latitude ) const
{
	day%= solar_days_in_year_;

	const float c_one_plus_eps= 1.0f + 0.01f;

	m_Vec3 hemisphere_local_rotation_axis= latitude > 0.0f ? rotation_axis_ : -rotation_axis_;
	m_Vec3 vec_from_planet_to_star_at_currend_day= GetVectorFromPlanetToStar( day * ticks_in_day_ );

	float angle_between_rotation_axis_and_direction_to_star=
		std::acos( hemisphere_local_rotation_axis * vec_from_planet_to_star_at_currend_day );

	float alpha= m_Math::FM_PI2 - latitude;
	float beta= m_Math::FM_PI2 - angle_between_rotation_axis_and_direction_to_star;

	float sin_equation= std::sin( beta ) / std::sin( alpha );
	if( std::abs(sin_equation) > c_one_plus_eps )
		return sin_equation > 0.0f ? 0.0f : float(ticks_in_day_);

	if( sin_equation > 1.0f ) sin_equation= 1.0f;
	else if( sin_equation < -1.0f ) sin_equation= -1.0f;

	float l= m_Math::FM_PI - 2.0f * std::asin( sin_equation ); // TODO: + or - before asin

	float sin_equation2= std::sin( l * 0.5f ) / std::cos( beta );
	if( std::abs(sin_equation2) > c_one_plus_eps )
	{
		// DO SOMETHING
		H_ASSERT(false);
		return 0;
	}
	if( sin_equation2 > 1.0f ) sin_equation2= 1.0f;
	else if( sin_equation2 < -1.0f ) sin_equation2= -1.0f;

	float night_duration=
		2.0f * std::asin( sin_equation2 );
	if( l > m_Math::FM_PI ) night_duration= m_Math::FM_2PI - night_duration;

	return (unsigned int)( night_duration / m_Math::FM_2PI * float(ticks_in_day_) );
}

m_Vec3 h_Calendar::GetLocalRotationAxis( float latitude ) const
{
	return m_Vec3(
		0.0f,
		std::cos(latitude),
		std::sin(latitude) );
}

// Maybe, unfinished.
// Can contain bugs.
float h_Calendar::GetSkySphereRotation( unsigned int ticks ) const
{
	ticks%= ticks_in_day_ * solar_days_in_year_;

	unsigned int ticks_in_stellar_day_= uint64_t(ticks_in_day_) * solar_days_in_year_ / (solar_days_in_year_ + 1);
	return -
		m_Math::FM_2PI *
		float(ticks) /
		float(ticks_in_stellar_day_);
}

// Maybe, unfinished.
// Can contain bugs.
m_Vec3 h_Calendar::GetSunVector( unsigned int ticks, float latitude ) const
{
	ticks%= ticks_in_day_ * solar_days_in_year_;

	unsigned int day= ticks / ticks_in_day_;
	unsigned int time= ticks % ticks_in_day_;

	m_Vec3 vec_from_planet_to_star_at_currend_day= GetVectorFromPlanetToStar( day * ticks_in_day_ );

	float angle_between_rotation_axis_and_direction_to_star=
		std::acos( rotation_axis_ * vec_from_planet_to_star_at_currend_day );

	float gamma= (angle_between_rotation_axis_and_direction_to_star - latitude);
	m_Vec3 local_sun_vector_at_midnight(
		0.0f,
		+std::cos(gamma),
		-std::sin(gamma) );

	m_Mat4 mat;
	mat.Rotate( GetLocalRotationAxis(latitude), -m_Math::FM_2PI * float(time) / float(ticks_in_day_) );

	return local_sun_vector_at_midnight * mat;
}

m_Vec3 h_Calendar::GetVectorFromPlanetToStar( unsigned int ticks ) const
{
	float angle= m_Math::FM_2PI * float(ticks) / float(solar_days_in_year_ * ticks_in_day_);

	return m_Vec3(
		-std::cos(angle),
		-std::sin(angle),
		0.0f);
}
