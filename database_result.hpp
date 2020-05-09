#ifndef __DATABASE_RESULT_HPP__
#define __DATABASE_RESULT_HPP__
#include <map>
#include <string>

class DatabaseResult {
public:
    virtual int field_count(void) = 0;
    virtual long num_rows(void) = 0;
    virtual const std::string& col_name(unsigned int col) = 0;
    virtual bool next_row(void) = 0;//preps the next row for access, returns true if one exists
    virtual const std::string& get_field(unsigned int col) = 0;//get the data from the specified column as a string, data is valid until next call to next_row()
    virtual bool field_is_null(unsigned int col) = 0;

    virtual ~DatabaseResult() {};
};

#endif
