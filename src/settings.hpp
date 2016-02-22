#pragma once
#include <map>
#include <string>

#include "fwd.hpp"

#include "hex.hpp"

class h_Settings
{
public:
	explicit h_Settings( const char* file_name );
	~h_Settings();

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
	Skonstruirovan tak, ctoby pri zapisj v "std::map" klüc hranilsä v "std::string", a pri polucenii znacenija iz "std::map"
	vyborka delalasj po "const char*", bez vydelenija pamäti, svojstvennogo "std::string".
	Pri pervom konstruirovanii sohranäjet u sebä syroj "const char*".
	V konstruktore kopirovanija perevodit "const char*" v "std::string".
	*/
	class h_SettingsStringContainer
	{
	public:
		explicit h_SettingsStringContainer( const char* str );
		h_SettingsStringContainer( const h_SettingsStringContainer& other );
		~h_SettingsStringContainer();

		explicit operator std::string() const;

		bool operator < ( const h_SettingsStringContainer& other ) const;

	private:
		void operator=(const h_SettingsStringContainer& other )= delete;

	private:
		const char* const c_str_;
		const std::string str_;
	};

	typedef std::map< h_SettingsStringContainer, std::string > MapType;

private:
	MapType map_;
	std::string file_name_;
};
