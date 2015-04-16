#pragma once
#include <map>
#include <string>

#include "hex.hpp"

class h_Settings
{

public:

	h_Settings();
	~h_Settings();

	void ReadFromFile( const char* file_name );
	void WriteToFile( const char* file_name ) const;

	void SetSetting( const char* name, const char* value );
	void SetSetting( const char* name, int value );
	void SetSetting( const char* name, bool value );
	void SetSetting( const char* name, float value );

	bool IsValue( const char* name ) const;

	//returns true, if can convert string to number
	bool IsNumber( const char* name ) const;

	const char* GetString( const char* name, const char* default_value = "" ) const;
	int GetInt( const char* name, int default_value = 0 ) const;
	float GetFloat( const char* name, float default_value = 0.0f ) const;
	bool GetBool( const char* name, bool default_value = false ) const;

private:


	/*
	Klass - obörtka nad "std::string" i "const char*".
	Skonstruirovan tak, ctoby pri zapisj v "std::map" klüc hranilsä v "std:;string", a pri polucenii znacenija iz "std::map"
	vyborka delalasj po "const char*", bez vydelenija pamäti, svojstvennogo "std:;string".
	Pri pervom konstruirovanii sohranäjet u sebä syroj "const char*".
	V konstruktore kopirovanija perevodit "const char*" v "std::string".
	*/
	class h_SettingsStringContainer
	{
	public:

		explicit h_SettingsStringContainer( const char* str );
		h_SettingsStringContainer( const h_SettingsStringContainer& other );
		~h_SettingsStringContainer();

		bool operator == ( const h_SettingsStringContainer& other) const;
		bool operator != ( const h_SettingsStringContainer& other) const;
		bool operator > ( const h_SettingsStringContainer& other) const;
		bool operator <=( const h_SettingsStringContainer& other) const;
		bool operator < ( const h_SettingsStringContainer& other) const;
		bool operator >= ( const h_SettingsStringContainer& other) const;

		private:
		std::string str_;
		const char* c_str_;
	};

	typedef std::map< h_SettingsStringContainer, std::string > MapType;
	MapType map_;

};
