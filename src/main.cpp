#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <cstdint>
#include <cassert>

#include "lua.hpp"

using namespace std;

struct Table
{
    vector<string> attrname;
    map<string, int> attrsize;// 2. define the BookName type
    map<string, multimap<string, int>> attrvar_i; //1.map<Name, multimap<Jack, back 1>>
    map<string, vector<string>> attrvar; // 3.map<bookname, 1> output: ICE
    map<string, multimap<int, int>> attrint_i;
    map<string, vector<int>> attrint;
    string primkey;
    int rownum;
};

map<string, struct Table*> tables;

void create_table(lua_State* L)
{
    struct Table* currtable = nullptr;
    // Get table name to create
    lua_pushstring(L, "table_name");
    lua_rawget(L, -2);
    if(tables.find(string(lua_tostring(L, -1))) == tables.end()) {
        currtable = tables[string(lua_tostring(L, -1))] = new Table();
    } else {
        cout << "error: ";
        cout << "Duplicate table name: " << string(lua_tostring(L, -1)) << endl;
        lua_pop(L, 1);
        return;
    }
    lua_pop(L, 1);
    // Get attribute list
    lua_pushstring(L, "attr");
    lua_rawget(L, -2);
    int attr_num = lua_objlen(L, -1);
    for(auto i = 1; i <= attr_num; i++) {
        lua_rawgeti(L, -1, i);
        currtable->attrname.push_back(string(lua_tostring(L, -1)));
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    // Get type list
    lua_pushstring(L, "attrsize");
    lua_rawget(L, -2);
    for(auto i = 1; i <= attr_num; i++) {
        lua_rawgeti(L, -1, i);
        currtable->attrsize[currtable->attrname[i-1]] = lua_tointeger(L, -1);
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
    // Get primary key
    lua_pushstring(L, "primattr");
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {
        // No primary key
        lua_pop(L, 1);
    } else {
        currtable->primkey = string(lua_tostring(L, -1));
        lua_pop(L, 1);
    }
    // Initialize row number
    currtable->rownum = 0;
    cout << "done." << endl;
}

void insert_into(lua_State* L)
{
    struct Table* currtable = nullptr;
    int value_num = 0;
    vector<string> attr_list;
    deque<string> str_v;
    deque<int> int_v;

    // Get table name to insert into
    lua_pushstring(L, "table_name");
    lua_rawget(L, -2);
    string table_name = string(lua_tostring(L, -1));
    if(tables.find(table_name) != tables.end()) {
        currtable = tables[string(lua_tostring(L, -1))];
        lua_pop(L, 1);
    } else {
        cout << "error: ";
        cout << "Table " << string(lua_tostring(L, -1));
        cout << " does not exist" << endl;
        lua_pop(L, 1);
        return;
    }
    // See if attribute list is provided
    lua_pushstring(L, "attr");
    lua_rawget(L, -2);
    if (lua_objlen(L, -1) == 0) {
        // No attribute list, assume same as schema
        attr_list = currtable->attrname;
        // Pop the empty attribute list
        lua_pop(L, 1);
    } else {
        // Get attribute list
        int attr_num = lua_objlen(L, -1);
        if (attr_num != currtable->attrsize.size()) {
            cout << "error: ";
            cout << "Attribute list length differs from table schema" << endl;
            lua_pop(L, 1);
            return;
        }
        for(int i = 0; i < attr_num; i++) {
            // Get attibutes
            lua_pushinteger(L, i+1);
            lua_gettable(L, -2);
            string name = lua_tostring(L, -1);
            if (currtable->attrsize.find(name) == currtable->attrsize.end()) {
                cout << "error: ";
                cout << "Attribute name " << name;
                cout << " does not exist in table " << table_name << endl;
                // Pop attribute name and list
                lua_pop(L, 2);
                return;
            }
            attr_list.push_back(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
        // Pop attribute list
        lua_pop(L, 1);
    }
    // Get value list
    lua_pushstring(L, "values");
    lua_rawget(L, -2);
    value_num = lua_objlen(L, -1);
    if (value_num != currtable->attrsize.size()) {
        cout << "error: ";
        cout << "Value list length differs from table schema" << endl;
        lua_pop(L, 1);
        return;
    }
    // Get value type list
    lua_pushstring(L, "valuetypes");
    lua_rawget(L, -3);
    vector<string> value_types;
    for(int i = 1; i <= value_num; i++) {
        // Get type
        lua_pushinteger(L, i);
        lua_gettable(L, -2);
        value_types.push_back(lua_tostring(L, -1));
        // Pop type
        lua_pop(L, 1);
    }
    // Pop value type list
    lua_pop(L, 1);
    // Get each value
    for(int i = 0; i < value_num; i++) {
        // Get value
        lua_pushinteger(L, i+1);
        lua_gettable(L, -2);
        if (currtable->attrsize[attr_list[i]] == -1) {
            // Ensure INT value
            if (!lua_isnumber(L, -1) || value_types[i] != "number") {
                cout << "error: Attribute ";
                cout << attr_list[i];
                cout << " is of INT type, cannot take value of `";
                cout << lua_tostring(L, -1) << "'" << endl;
                // Pop value and value list
                lua_pop(L, 2);
                return;
            }
            // Check bounds of INT value
            if (lua_tonumber(L, -1) > 2147483647) {
                cout << "error: Value ";
                cout << static_cast<int64_t>(lua_tonumber(L, -1));
                cout << " is too large for INT type" << endl;
                // Pop value and value list
                lua_pop(L, 2);
                return;
            } else if (lua_tonumber(L, -1) < -2147483648) {
                cout << "error: Value ";
                cout << static_cast<int64_t>(lua_tonumber(L, -1));
                cout << " is too small for INT type" << endl;
                // Pop value and value list
                lua_pop(L, 2);
                return;
            }
            int integer = lua_tointeger(L, -1);
            // Check for duplicate primary key
            if (attr_list[i] == currtable->primkey && 
                currtable->attrint_i[attr_list[i]].find(integer)!=
                currtable->attrint_i[attr_list[i]].end()){
                cout << "error: Primary key value " << integer;
                cout << " exists in table ";
                cout << table_name << endl;
                // Pop value and value list
                lua_pop(L, 2);
                return;
            }
            int_v.push_back(integer);
        } else {
            // Ensure VARCHAR value
            if (!lua_isstring(L, -1) || value_types[i] != "string") {
                cout << "error: Attribute ";
                cout << attr_list[i];
                cout << " is of VARCHAR type, cannot take value of ";
                cout << lua_tointeger(L, -1) << endl;
                // Pop value and value list
                lua_pop(L, 2);
                return;
            }
            string str = lua_tostring(L, -1);
            // Check VARCHAR size
            if (str.length() > currtable->attrsize[attr_list[i]]) {
                cout << "error: String `" << lua_tostring(L, -1);
                cout << "' too long for attribute " << attr_list[i] << endl;
                // Pop value and value list
                lua_pop(L, 2);
                return;
            }
            // Check for duplicate primary key
            if (attr_list[i] == currtable->primkey && 
                currtable->attrvar_i[attr_list[i]].find(str)!=
                currtable->attrvar_i[attr_list[i]].end()){
                cout << "error: Primary key value " << "`" << str << "'";
                cout << " exists in table ";
                cout << table_name << endl;
                // Pop value and value list
                lua_pop(L, 2);
                return;
            }
            str_v.push_back(str);
        }
        // Pop value
        lua_pop(L, 1);
    }
    // Pop value list
    lua_pop(L, 1);    
    // Commit to table
    for(int i = 0; i < value_num; i++) {
        string attr_name = attr_list[i];
        if (currtable->attrsize[attr_name] == -1) {
            currtable->attrint[attr_name].push_back(int_v.front());
            currtable->attrint_i[attr_name].insert(
                pair<int,int>(int_v.front(), currtable->attrint.size()-1));
            int_v.pop_front();
        } else {
            currtable->attrvar[attr_name].push_back(str_v.front());
            currtable->attrvar_i[attr_name].insert(
                pair<string,int>(str_v.front(), currtable->attrvar.size()-1));
            str_v.pop_front();
        }
    }
    currtable->rownum++;
    cout << "done." << endl;
}
/* Caution: The attrname should be in uppercase! */
struct Table* new_table(vector<string> attrname, vector<int> attrsize)
{
    /* Attribute must have both name and size */
    assert(attrname.size() == attrsize.size());

    struct Table *currtable = new Table();
    /* Attribute names */
    currtable->attrname = attrname;
    /* Attribute sizes */
    for(auto i = 0; i < currtable->attrname.size(); i++) {
        currtable->attrsize[currtable->attrname[i]] = attrsize[i];
    }
    /* Number of rows */
    currtable->rownum = 0;

    return currtable;
}

/* Caution:
 *   Provide the values in schema order!
 *   Use push_back() on int_v and str_v in order!
 *   Check insert_into() for more information.
 */
void new_row(struct Table* table, deque<int> int_v, deque<string> str_v)
{
    struct Table* currtable = table;

    /* Must provide right number of values */
    assert(currtable->attrsize.size() == int_v.size() + str_v.size());

    /* Insert the row now */
    for(string attr_name: currtable->attrname) {
        if (currtable->attrsize[attr_name] == -1) {
            currtable->attrint[attr_name].push_back(int_v.front());
            currtable->attrint_i[attr_name].insert(
                pair<int,int>(int_v.front(), currtable->attrint.size()-1));
            int_v.pop_front();
        } else {
            currtable->attrvar[attr_name].push_back(str_v.front());
            currtable->attrvar_i[attr_name].insert(
                pair<string,int>(str_v.front(), currtable->attrvar.size()-1));
            str_v.pop_front();
        }
    }
    /* Increment row number */
    currtable->rownum++;
}

void print_table(struct Table *table)
{
    // Print attributes
    for(auto str:table->attrname) {
        if (str == table->primkey)
            printf("%15s*(%2d)", str.c_str(), table->attrsize[str]);
        else
            printf("%16s(%2d)", str.c_str(), table->attrsize[str]);
    }
    cout << endl;
    // Print each row
    // Cache the types to make it efficient
    vector<int> attr_size;
    deque<vector<int>> attr_int;
    deque<vector<string>> attr_var;
    for(auto str:table->attrname) {
        attr_size.push_back(table->attrsize[str]);
        if (attr_size.back() == -1)
            attr_int.push_back(table->attrint[str]);
        else
            attr_var.push_back(table->attrvar[str]);
    }
    for(int i = 0; i < table->rownum; i++) {
        for(int j = 0; j < attr_size.size(); j++) {
            if (attr_size[j] == -1) {
                printf("%20d", attr_int.front()[i]);
                attr_int.push_back(attr_int.front());
                attr_int.pop_front();
            } else {
                printf("%20s", attr_var.front()[i].c_str());
                attr_var.push_back(attr_var.front());
                attr_var.pop_front();
            }
        }
        cout << endl;
    }
}

void select (lua_State* L)
{	
	int command_num = lua_objlen(L,-1);
	
	vector<string> SEL_alias;   				// include '*'
	vector<string> SEL_target;					// all select target list
	vector<string> opt;
	
	vector<string> From_table_name;			
	vector<string> From_alias;
	
	vector<string> where_condition_first_alias; // it can be alias name, which occurs in this condition(where_condition_first_attrname is not empty), or attribute name.
	vector<string> where_condition_first_attrname;
	vector<string> where_condition_second_alias;
	vector<string> where_condition_second_attrname;
	vector<string> logical_op;
	string op_flag_first;
	string op_flag_second;
	string compare_char_first;
	string compare_char_second;
	int compare_int_first;
	int compare_int_second;
	
	
	for (auto i=2;i<=command_num;i++){
		lua_rawgeti(L,-1,i);
		int check_elem = lua_objlen(L,-1);
		if (i==2) {													//distinguish from "SELECT"(i=2), "From"(i=3), and "Where" query(i=4)
			for (auto j=1;j<=check_elem;j++){
				lua_rawgeti(L,-1,j);			
				if (j>=2){
					int check_subelem = lua_objlen(L,-1);
					for (auto k=1;k<=check_subelem;k++) {
						if (k==1){
							lua_rawgeti(L,-1,k);
							SEL_alias.push_back(string(lua_tostring(L, -1)));
							lua_pop(L,1);
						} else{
							lua_rawgeti(L,-1,k);
							SEL_target.push_back(string(lua_tostring(L, -1)));
							lua_pop(L,1);
						}
					}
				} else {
					opt.push_back(string(lua_tostring(L, -1))); 		// opt = COUNT or SUM or attr or null
				}
				lua_pop(L,1);
			} 
		} else if (i==3) {			
			for (auto j=1;j<=check_elem;j++){
				lua_rawgeti(L,-1,j);
				int check_subelem = lua_objlen(L,-1);
				for (auto k=1;k<=check_subelem;k++) {
					if (k==1){
						lua_rawgeti(L,-1,k);
						From_table_name.push_back(string(lua_tostring(L, -1)));
						lua_pop(L,1);
					}else {
						lua_rawgeti(L,-1,k);
						From_table_name.push_back(string(lua_tostring(L, -1)));
						lua_pop(L,1);
					}
				}
				lua_pop(L,1);
			}
		} else {
			for (auto j=1;j<=check_elem;j++){
				lua_rawgeti(L,-1,j);
				int check_subelem = lua_objlen(L,-1);
				if (j==2){
					auto logical_op = string(lua_tostring(L, -1));  // logical_op = "AND" or "OR"
					cout<<logical_op<<endl;
				} else {
					for (auto k=1;k<=check_subelem;k++) {
						lua_rawgeti(L,-1,k);
						int check_alias = lua_objlen(L,-1);
						for (auto it=1;it<=check_alias;it++) {
							lua_rawgeti(L,-1,it);
							if (j ==1 && k==1 && it ==1){
								where_condition_first_alias.push_back((lua_tostring(L, -1)));
								lua_pop(L,1);
							}else if (j == 1 && k==1 && it ==2){
								where_condition_first_attrname.push_back((lua_tostring(L, -1)));
								lua_pop(L,1);
							}else if (j == 1 && k==2 && it == 1){
								op_flag_first = lua_tostring(L, -1);
								lua_pop(L,1);
							}else if (j == 1 && k==3 && it ==1){
								compare_char_first = lua_tostring(L, -1);
								lua_pop(L,1);
							}else if (j ==3 && k==1 && it ==1){
								where_condition_second_alias.push_back((lua_tostring(L, -1)));
								lua_pop(L,1);
							}else if (j==3 && k ==1 && it ==2){
								where_condition_second_attrname.push_back((lua_tostring(L, -1)));
								lua_pop(L,1);
							}else if (j==3 && k ==2 && it ==1){
								op_flag_second = lua_tostring(L, -1);
								lua_pop(L,1);
							}else if (j==3 && k ==3 && it ==1){
								compare_char_second = lua_tostring(L, -1);
								lua_pop(L,1);
							}
						}
						lua_pop(L,1);
					}
				}
				lua_pop(L,1);
			}
			
		}
		lua_pop(L,1);
	}
	
	/*
	struct Table* currtable = nullptr;
	
	if(tables.find(From_table_name[0]) != tables.end()) {
        currtable = tables[From_table_name[0]];
    }
	vector<string> target_list;   // name of target_list;
	vector<int> target_size;	  // size of target_list;
    deque<string> str_v;
    deque<int> int_v;
	map<string,string> alias_name_to_table;				//{key,value} = {alias, table_name};
	vector<int> tmp_i;									// in order to transform vector to deque
	vector<string> tmp_v;
	
	
	
	
	
	
	//This can run without where condition.
	/*
	if (SEL_alias[0] == "*"){
		// get the table name (From_table_name[0]), finding the table's attrlist and attrsize
		target_list = currtable->attrname;
		int target_list_number = target_list.size();
		for (int in =0; in<target_list_number; in++){
			int value = currtable-> attrsize[target_list[in]];
			target_size.push_back(value);
		}
	}else{
		int target_number = SEL_alias.size();
		for (int i=0;i<target_number;i++){
			auto tmp = SEL_alias[i];
			target_list.push_back(tmp);
			int value = currtable-> attrsize[tmp];
			target_size.push_back(value);
		}
	}
	auto selecttable = new_table(target_list,target_size);
	int target_list_number = target_list.size();

	for(int i = 0; i < currtable->rownum; i++) {
		for(int in =0; in< target_list_number; in++){
			int value = target_size[in];
			if (value == -1){
				tmp_i = currtable->attrint[target_list[in]];
				int_v.push_back(tmp_i[i]);
			}else{
				tmp_v = currtable->attrvar[target_list[in]];
				str_v.push_back(tmp_v[i]);
			}
		}
		new_row(selecttable, int_v, str_v);
		for(int in =0; in< target_list_number; in++){
			int value = target_size[in];
			if (value == -1){
				int_v.pop_front();
			}else{
				str_v.pop_front();
			}
		}
	}
	print_table(selecttable);
	*/
	
	
}




void print_tables()
{
    cout << endl << "Tables:" << endl;
    for(auto it = tables.begin(); it != tables.end(); ++it)
    {
        // Print table name
        cout << "[" << it->first << "]" << endl;
        auto table = it->second;
        // Print attributes
        for(auto str:table->attrname) {
            if (str == table->primkey)
                printf("%15s*(%2d)", str.c_str(), table->attrsize[str]);
            else
                printf("%16s(%2d)", str.c_str(), table->attrsize[str]);
        }
        cout << endl;
        // Print each row
        // Cache the types to make it efficient
        vector<int> attr_size;
        deque<vector<int>> attr_int;
        deque<vector<string>> attr_var;
        for(auto str:table->attrname) {
            attr_size.push_back(table->attrsize[str]);
            if (attr_size.back() == -1)
                attr_int.push_back(table->attrint[str]);
            else
                attr_var.push_back(table->attrvar[str]);
        }
        for(int i = 0; i < table->rownum; i++) {
            for(int j = 0; j < attr_size.size(); j++) {
                if (attr_size[j] == -1) {
                    printf("%20d", attr_int.front()[i]);
                    attr_int.push_back(attr_int.front());
                    attr_int.pop_front();
                } else {
                    printf("%20s", attr_var.front()[i].c_str());
                    attr_var.push_back(attr_var.front());
                    attr_var.pop_front();
                }
            }
            cout << endl;
        }
    }
}


int main(int argc, char* argv[])
{
	char* inputfilename = nullptr;
    if (argc < 2) {
        cout << "Usage: " << argv[0] << " <Input File>" << endl;
        return 0;
    } else {
        inputfilename = argv[1];
    }

    lua_State *L = luaL_newstate();
    luaL_openlibs(L);

    int status = luaL_loadfile(L, "src/parser.lua");
    if (status) {
        // If something went wrong, error message is at the top of 
        // the stack 
        cout << lua_tostring(L, -1) << endl;
        exit(1);
    }
    int result = lua_pcall(L, 0, 1, 0);
    if (result) {
        cout << lua_tostring(L, -1) << endl;
        lua_pop(L, 1);
    }

    lua_getglobal(L, "parseCommand");
    lua_pushnil(L);
    lua_pushstring(L, inputfilename);

    while (1) {
        result = lua_pcall(L, 2, 1, 0);
        if (result) {
            cout << lua_tostring(L, -1) << endl;
            lua_pop(L, 1);
        } else {
            int command_num = lua_objlen(L, -1);
            for (auto i = 1; i <= command_num; i++) {
                // Get a command
                lua_rawgeti(L, -1, i);
                // Get operation
                lua_pushstring(L, "op");
                lua_rawget(L, -2);
                auto op = string(lua_tostring(L, -1));
                lua_pop(L, 1);
                if (op == "CREATE TABLE") {
                    cout << "Creating table...";
                    create_table(L);
                } else if (op == "INSERT INTO") {
                    cout << "Inserting row...";
                    insert_into(L);
                } else if (op == "SELECT") {
                    // TODO
					printf("SELECT is not yet implemented!\n");
					select(L);
                } else {
                    cout << "Unknown operation " << op << endl;
                }
                // Pop command
                lua_pop(L, 1);
            }
            // Pop command list
            lua_pop(L, 1);
        }

        print_tables();

        cout << "\n>> ";
        string buf;
        std::getline (std::cin, buf);
        if(buf[0] == 'q') {
            return 0;
        }else if (buf[0] == 'i') {
			lua_getglobal(L, "parseCommand");
			lua_pushnil(L);
			std::getline (std::cin, buf);
			lua_pushstring(L, buf.c_str());
			
		}
		else {
            lua_getglobal(L, "parseCommand");
            lua_pushstring(L, buf.c_str());
            lua_pushnil(L);
        }
    }

    lua_close(L);
}