local lpeg = require('lpeg')
local dump = require('pl.pretty').dump

local match = lpeg.match
local P = lpeg.P
local C = lpeg.C
local Ct = lpeg.Ct
local Cc = lpeg.Cc

local l = {}
lpeg.locale(l)

local function sp(pattern)  -- Surround pattern with space
  local space = l.space^0  -- One or more spaces
  return space * pattern * space
end

local input = {
'SELECT COUNT(*) FROM Student',
'SELECT SUM(salary) FROM Employee',
"SELECT name FROM student",
"SELECT name FROM student AS s",
'SELEcT e.name, s.name FROM student AS s, employee AS e',
'select en.name, su.ei FROM student as s2, wqe as q4',
'SELECT departmentId, name FROM Department WHERE departmentId > 1000 AND buildingNo = 101',
'SELECT departmentId, name FROM Department as d WHERE d.departmentId > 1000 AND d.buildingNo = 101'
}

local function upper(str)  -- dirty uppercase function, ignores string literals
  -- String
  local s = C(("'" * (1-P"'")^0 * "'") + ('"' * (1-P'"')^0 * '"'))
  -- Command outside strings, capture uppercase
  local c = ((1-(P"'"+P'"'))^1)/string.upper
  -- Concat function to fold
  local function concat(s1, s2) return s1..s2 end
  -- Concatenate substrings
  local statement = lpeg.Cf( (s+c)^0 , concat)
  return statement:match(str)
end

local iden             = sp(C((l.alpha+P'_') * (l.alnum+P'_')^0))
local asterisk         = sp(Ct(C(P'*')))
local str              = sp(("'" * C((1-P"'")^0) * "'") + ('"' * C((1-P'"')^0) * '"'))
local int              = sp(C(l.digit^1))

local     alias            = iden
local     table_name       = iden
local   prefix           = alias + table_name
local   attr             = iden
local attr_name        = sp(Ct((prefix * '.') ^ -1 * attr))
local attr_asterisk    = sp(Ct((prefix * '.') ^ -1 * C(P'*')))
local target           = attr_name + attr_asterisk

local   count_fun        = sp(C(P'COUNT') * sp('(') * ( attr_name + asterisk ) * sp(')'))
local   sum_fun          = sp(C(P'SUM')   * sp('(') * ( attr_name + asterisk ) * sp(')'))
local   attr_list        = sp(Cc'ATTR' * target * ( ',' * target ) ^ 0)
local target_list      = Ct(count_fun + sum_fun + attr_list + Cc'null'*asterisk)

local   table_entry      = Ct(table_name * ( 'AS' * alias ) ^ -1)
local table_list       = Ct(table_entry * ( ',' * table_entry ) ^ 0)

local     op_cmp           = sp(Cc('eq')*P'=')
                           + sp(Cc('ne')*P'<>')
                           + sp(Cc('lt')*P'<')
                           + sp(Cc('gt')*P'>')
local     expr             = attr_name + Ct(str) + Ct(int)
local   bool_expr        = Ct(expr * Ct(op_cmp) * expr)
local   op_bool          = sp(C(P'AND')) + sp(C(P'OR'))
local where_clause     = Ct(P'WHERE' * bool_expr * (op_bool * bool_expr)^-1)

local select_statement = Ct(Cc'SELECT' * sp(P'SELECT' * target_list * P'FROM' * table_list * where_clause^-1))

function parseSelect(stmt)
  print(upper(stmt))
  local result = select_statement:match(upper(stmt))
  local alias = {}
  
  -- Get alias
  for i,v in ipairs(result[3]) do
    if #v == 2 then
      if alias[v[2]] ~= nil then error(string.format("Duplicate alias %s", v[2])) end
      alias[v[2]] = v[1]
    end
  end

  result.op = result[1]

  return result
end

-- Test function
--[[
for i,v in ipairs(input) do
  local result = parse_select(v)
  dump(result)
end
]]
