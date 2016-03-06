#include <cmath>

#include "test.h"

#include "../calendar.hpp"
#include "../math_lib/math.hpp"

H_TEST(CalendarLocalRotationAxisTest)
{
	unsigned int ticks_in_day= 1000;
	unsigned int days_in_year= 45;
	float rotation_axis_angle= 0.0f;
	unsigned int sumer_solitice_day= 20;
	h_Calendar calendar(
		ticks_in_day,
		days_in_year,
		rotation_axis_angle,
		sumer_solitice_day);

	float eps= 0.01f;

	// Equator
	H_TEST_EXPECT( (
		calendar.GetLocalRotationAxis(0.0f) -
		m_Vec3(0.0f, 1.0f, 0.0f) ).Length() <= eps );

	// North hemisphere
	H_TEST_EXPECT( (
		calendar.GetLocalRotationAxis(m_Math::pi_4) -
		m_Vec3(0.0f, std::cos(m_Math::pi_4), std::sin(m_Math::pi_4)) ).Length() <= eps );

	// South hemisphere
	H_TEST_EXPECT( (
		calendar.GetLocalRotationAxis(-m_Math::pi_6) -
		m_Vec3(0.0f, std::cos(-m_Math::pi_6), std::sin(-m_Math::pi_6)) ).Length() <= eps );

	// North pole
	H_TEST_EXPECT( (
		calendar.GetLocalRotationAxis(m_Math::pi_2) -
		m_Vec3(0.0f, 0.0f, 1.0f) ).Length() <= eps );
}

H_TEST(CalendarSunPositionTest)
{
	unsigned int ticks_in_day= 64;
	unsigned int days_in_year= 72;
	float rotation_axis_angle= 15.35f * m_Math::deg2rad;
	unsigned int sumer_solitice_day= 22;
	h_Calendar calendar(
		ticks_in_day,
		days_in_year,
		rotation_axis_angle,
		sumer_solitice_day);

	float local_latitude= 25.0f * m_Math::deg2rad;

	// In middle of the day sun z must be positive.
	H_TEST_EXPECT( calendar.GetSunVector(ticks_in_day * 10 + ticks_in_day / 2, local_latitude ).z > 0.0f );

	// In middle of the night sun z must be negative.
	H_TEST_EXPECT( calendar.GetSunVector(ticks_in_day * 65, local_latitude ).z < 0.0f );

	// In first part of the day sun must be in the east.
	H_TEST_EXPECT( calendar.GetSunVector(ticks_in_day * 9 + ticks_in_day / 4, local_latitude ).x > 0.0f );

	// In second part of the day sun must be in the east.
	H_TEST_EXPECT( calendar.GetSunVector(ticks_in_day * 37 + ticks_in_day * 3 / 4, local_latitude ).x < 0.0f );

	{
		unsigned int midday_ticks= ticks_in_day * 5 + ticks_in_day / 2;
		float z_on_midday= calendar.GetSunVector( midday_ticks, local_latitude ).z;
		float z_before_midday= calendar.GetSunVector( midday_ticks - ticks_in_day / 8, local_latitude ).z;
		float z_after_midday = calendar.GetSunVector( midday_ticks + ticks_in_day / 8, local_latitude ).z;

		// Sun must reach hightest altitude at midday.
		H_TEST_EXPECT( z_on_midday > z_before_midday );
		H_TEST_EXPECT( z_on_midday > z_after_midday );
	}

	{
		unsigned int sumer_solitice_day_ticks= sumer_solitice_day * ticks_in_day + ticks_in_day / 2;
		float z_on_summer_solitice_midday= calendar.GetSunVector( sumer_solitice_day_ticks, local_latitude ).z;
		float z_before= calendar.GetSunVector( sumer_solitice_day_ticks - ticks_in_day, local_latitude ).z;
		float z_after = calendar.GetSunVector( sumer_solitice_day_ticks + ticks_in_day, local_latitude ).z;

		// Sun must reach hightest altitude at midday at summer solitice day.
		H_TEST_EXPECT( z_on_summer_solitice_midday > z_before );
		H_TEST_EXPECT( z_on_summer_solitice_midday > z_after );
	}
	{
		unsigned int winter_solitice_day_ticks= ( sumer_solitice_day + days_in_year / 2 )* ticks_in_day + ticks_in_day / 2;
		float z_on_winter_solitice_midday= calendar.GetSunVector( winter_solitice_day_ticks, local_latitude ).z;
		float z_before= calendar.GetSunVector( winter_solitice_day_ticks - ticks_in_day, local_latitude ).z;
		float z_after = calendar.GetSunVector( winter_solitice_day_ticks + ticks_in_day, local_latitude ).z;

		// Sun must reach lowest altitude at midday at winter solitice day.
		H_TEST_EXPECT( z_on_winter_solitice_midday < z_before );
		H_TEST_EXPECT( z_on_winter_solitice_midday < z_after );
	}
}
