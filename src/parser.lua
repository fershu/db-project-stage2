local lexer = require('pl.lexer')
local List = require('pl.List')
local stringx = require('pl.stringx')
stringx.import()
dofile("src/select_parser.lua")

-- Input is file or not
local isfile
-- Current filename
local filename
-- Temp values used in parser
local t,v,vu
-- State graph
local states = {}
-- Parsed command
local command
-- Result commands
local results = {}

states = {
  ["start"] = function (tok)
    command = {}
    command.op = nil
    command.table_name = nil
    command.attr = List()
    command.attrtype = List()
    command.attrsize = List()
    command.values = List()
    command.valuetypes = List()
    command.lastattr = nil
    command.primattr = nil
    t,v = tok()
    -- Empty statement
    if t == nil then
      return results
    end
    vu = string.upper(v)
    -- Expect keywords CREATE or INSERT
    if t == "iden" then
      if vu == "CREATE" then
        return states["CREATE->TABLE"](tok)
      elseif vu == "INSERT" then
        return states["INSERT->INTO"](tok)
      elseif vu == "SELECT" then
        local select_stmt = ""
        local select_cmd

        select_stmt = v.." "..lexer.getrest(tok)
        select_cmd = parseSelect(select_stmt)
        if select_cmd then
          table.insert(results, select_cmd)
          if t then return states.start(tok) else return results end
        else
          return states.error(tok, "SEARCH error")
        end
      end
    end
    return states.error(tok,
      "Expecting keywords CREATE or INSERT, got: ", v)
  end,
  ["error"] = function (tok, msg, v)
    local line_number
    -- Show line number if working on a file
    if isfile then
      line_number = filename..":"..lexer.lineno(tok)..": "
    else
      line_number = ""
    end
    -- Print error message
    if msg == nil and v == nil then
      error(line_number.."Unexpected end of SQL statement", 2)
    elseif msg == nil and v ~= nil then
      error(line_number.."Unexpected token: ", v, 2)
    elseif v ~= nil then
      error(line_number..msg..v, 2)
    else
      error(line_number..msg, 2)
    end
  end,
  ["CREATE->TABLE"] = function (tok)
    t,v = tok()
    -- Expect keyword TABLE
    if t == "iden" and string.upper(v) == "TABLE" then
      command.op = "CREATE TABLE"
      return states.table_name(tok)
    else
      return states.error(tok,
        "Expecting keyword TABLE, got: ", v)
    end
  end,
  ["table_name"] = function (tok)
    t,v = tok()
    if t == "iden" then
      command.table_name = string.upper(v)
    else
      return states.error(tok,
        "Expecting table name, got: ",v)
    end
    t,v = tok()
    if command.op == "CREATE TABLE" then
      if t == "(" then
        return states.attr_name(tok)
      else
        return states.error(tok,
          "Expecting `(' to start schema definition, got: ", v)
      end
    elseif command.op == "INSERT INTO" then
      if t == "(" then
        return states.attr_name(tok)
      elseif t == "iden" and string.upper(v) == "VALUES" then
        return states.value_list(tok)
      else
        return states.error(tok,
          "Expecting attribute or value list, got: ", v)
      end
    end
  end,
  ["attr_name"] = function (tok)
    t,v = tok()
    if t == "iden" then
      vu = string.upper(v)
      -- Check duplicated attribute
      if not command.attr:contains(vu) then
        command.attr:append(vu)
        if command.op == "CREATE TABLE" then
          command.lastattr = vu
          return states.attr_type(tok)
        elseif command.op == "INSERT INTO" then
          t,v = tok()
          if t == "," then
            return states.attr_name(tok)
          elseif t == ")" then
            t,v = tok()
            if t == "iden" and string.upper(v) == "VALUES" then
              return states.value_list(tok)
            else
              return states.error(tok,
                "Expecting keyword VALUES, got: ", v)
            end
          else
            return states.error(tok,
              "Expecting `,' or `)', got: ", v)
          end
        else
          error(command.op.." NOT IMPLEMENTED")
        end
      else
        return states.error(tok,
          "Duplicate attribute name: ", vu)
      end
    else
      return states.error(tok,
        "Expecting attribute name, got: ", v)
    end
  end,
  ["attr_type"] = function (tok)
    t,v = tok()
    vu = string.upper(v)
    if vu == "INT" then
      command.attrtype:append("INT")
      command.attrsize:append(-1)
      return states.primary_key(tok)
    elseif vu == "VARCHAR" then
      command.attrtype:append("VARCHAR")
      return states.varchar_size(tok)
    else
      return states.error(tok,
        "Expecting known attribute type name, got: ", v)
    end
  end,
  ["varchar_size"] = function (tok)
    t,v = tok()
    if t ~= "(" then
      return states.error(tok,
        "Expecting `(' and VARCHAR size, got: ", v)
    end
    t,v = tok()
    if t == "number" then
      if v == math.floor(v) then
        command.attrsize:append(v)
        t,v = tok()
        if t == ")" then
          return states.primary_key(tok)
        else
          return states.error(tok,
            "Expecting `)', got: ", v)
        end
      else
        return states.error(tok,
          "VARCHAR size not integer, got: ", v)
      end
    else
      return states.error(tok,
        "VARCHAR size not number, got: ", v)
    end
  end,
  ["primary_key"] = function (tok)
    t,v = tok()
    -- Optional keyword PRIMARY KEY
    if t == "iden" then
      if string.upper(v) == "PRIMARY" then
        t,v = tok()
        if string.upper(v) == "KEY" then
          if command.primattr == nil then
            command.primattr = command.lastattr
          else
            return states.error(tok,
              "Cannot accept multiple primary keys")
          end
          t,v = tok()
        else
          return states.error(tok,
            "Expecting keyword KEY, got: ", v)
        end
      end
    end
    -- Next attribute or end of attribute list
    if t == "," then
      return states.attr_name(tok)
    elseif t == ")" then
      return states.create_table_accept(tok)
    end
    return states.error(tok,
      "Expecting keyword PRIMARY or next attribute, got: ", v)
  end,
  ["create_table_accept"] = function (tok)
    table.insert(results, command)
    t,v = tok()
    if t == ";" then
      return states.start(tok)
    elseif t == nil then
      return results
    else
      return states.error(tok,
        "Extra token `"..v.."', maybe a semicolon is missing?")
    end
  end,
  ["INSERT->INTO"] = function (tok)
    t,v = tok()
    -- Expect keyword INTO
    if t == "iden" and string.upper(v) == "INTO" then
      command.op = "INSERT INTO"
      return states.table_name(tok)
    else
      return states.error(tok,
        "Expecting keyword `INTO', got: ", v)
    end
  end,
  ["value_list"] = function (tok)
    t,v = tok()
    if t == "(" then
      return states.value(tok)
    else
      return states.error(tok,
        "Expecting value list, got: ", v)
    end
  end,
  ["value"] = function (tok)
    t,v = tok()
    -- Expect INT or VARCHAR literal
    if t == "string" or t == "number" then
      if t == "number" and v ~= math.floor(v) then
        return states.error(tok,
          "Expecting integer literal, got: ", v)
      end
      command.valuetypes:append(t)
      command.values:append(v)
      t,v = tok()
      -- Expect next literal or end of list
      if t == "," then
        return states.value(tok)
      elseif t == ")" then
        return states.insert_into_accept(tok)
      else
        return states.error(tok,
          "Expecting `,' or `)', got: ", v)
      end
    else
      return states.error(tok,
        "Expecting INT or VARCHAR literal, got: ", v)
    end
  end,
  ["insert_into_accept"] = function (tok)
    if #(command.attr) == #(command.values) or #(command.attr) == 0 then
      table.insert(results, command)
    else
      return states.error(tok,
        "Attribute number and value number mismatch")
    end
    t,v = tok()
    if t == ";" then
      return states.start(tok)
    elseif t == nil then
      return results
    else
      return states.error(tok,
        "Extra token `"..v.."', maybe a semicolon is missing?")
    end
  end
}

-- Seperate str using c
local function seperate(str, c)
  local lpeg = require('lpeg')
  local match = lpeg.match
  local P = lpeg.P
  local C = lpeg.C
  local Ct = lpeg.Ct

  local seperator = P(c)
  local rest = C((P(1)-seperator)^0)
  local all = rest * (seperator * rest)^0 * seperator ^ 0
  return match(Ct(all), str)
end

function parseCommand(input, file)
  -- Clear previous result
  results = {}
  
  if file then
    -- Enable line number in error message
    isfile = 1
    filename = file
    local f,err = io.open(file, "r")
    input = f:read("*all")
    if(input == nil) then error(err) end
  else
    isfile = nil
  end

  for i,v in ipairs(seperate(input,';')) do
    tok = lexer.scan(v)
    states.start(tok)
  end

  return results
end
