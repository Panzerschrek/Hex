#include <cmath>

#include "math_lib/m_math.h"
#include "math_lib/assert.hpp"

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

unsigned int h_Calendar::GetNightLength( unsigned int day, float latitude ) const
{
	day%= solar_days_in_year;

	const float c_one_plus_eps= 1.0f + 0.01f;

	float day_angle= m_Math::FM_2PI * float(day) / float(solar_days_in_year_);
	m_Vec3 hemisphere_local_rotation_axis= latitude > 0.0f ? rotation_axis_ : -rotation_axis_;

	m_Vec3 vec_from_planet_to_star_at_currend_day(
		-std::cos(day_angle),
		-std::sin(day_angle),
		0.0f);

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

	//h_Console::Info(
	//	"axis angle: ", angle_between_rotation_axis_and_direction_to_star * 180.0f / m_Math::FM_PI,
	//	" l: ", l );

	return (unsigned int)( night_duration / m_Math::FM_2PI * float(ticks_in_day_) );
}
