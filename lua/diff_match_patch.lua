--[[
* Diff Match and Patch
* Copyright 2018 The diff-match-patch Authors.
* https://github.com/google/diff-match-patch
*
* Based on the JavaScript implementation by Neil Fraser.
* Ported to Lua by Duncan Cross.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
--]]

--[[
-- Lua 5.1 and earlier requires the external BitOp library.
-- This library is built-in from Lua 5.2 and later as 'bit32'.
require 'bit'   -- <https://bitop.luajit.org/>
local band, bor, lshift
    = bit.band, bit.bor, bit.lshift
--]]

local band, bor, lshift
    = bit32.band, bit32.bor, bit32.lshift
local type, setmetatable, ipairs, select
    = type, setmetatable, ipairs, select
local unpack, tonumber, error
    = unpack, tonumber, error
local strsub, strbyte, strchar, gmatch, gsub
    = string.sub, string.byte, string.char, string.gmatch, string.gsub
local strmatch, strfind, strformat
    = string.match, string.find, string.format
local tinsert, tremove, tconcat
    = table.insert, table.remove, table.concat
local max, min, floor, ceil, abs
    = math.max, math.min, math.floor, math.ceil, math.abs
local clock = os.clock


-- Utility functions.

local percentEncode_pattern = '[^A-Za-z0-9%-=;\',./~!@#$%&*%(%)_%+ %?]'
local function percentEncode_replace(v)
  return strformat('%%%02X', strbyte(v))
end

local function indexOf(a, b, start)
  if (#b == 0) then
    return nil
  end
  return strfind(a, b, start, true)
end

local htmlEncode_pattern = '[&<>\n]'
local htmlEncode_replace = {
  ['&'] = '&amp;', ['<'] = '&lt;', ['>'] = '&gt;', ['\n'] = '&para;<br>'
}

-- Public API Functions
-- (Exported at the end of the script)

local diff_main,
      diff_cleanupSemantic,
      diff_cleanupEfficiency,
      diff_levenshtein,
      diff_prettyHtml

local match_main

local patch_make,
      patch_toText,
      patch_fromText,
      patch_apply

--[[
* The data structure representing a diff is an array of tuples:
* {{DIFF_DELETE, 'Hello'}, {DIFF_INSERT, 'Goodbye'}, {DIFF_EQUAL, ' world.'}}
* which means: delete 'Hello', add 'Goodbye' and keep ' world.'
--]]
local DIFF_DELETE = -1
local DIFF_INSERT = 1
local DIFF_EQUAL = 0

-- Number of seconds to map a diff before giving up (0 for infinity).
local Diff_Timeout = 1.0
-- Cost of an empty edit operation in terms of edit characters.
local Diff_EditCost = 4
-- At what point is no match declared (0.0 = perfection, 1.0 = very loose).
local Match_Threshold = 0.5
-- How far to search for a match (0 = exact location, 1000+ = broad match).
-- A match this many characters away from the expected location will add
-- 1.0 to the score (0.0 is a perfect match).
local Match_Distance = 1000
-- When deleting a large block of text (over ~64 characters), how close do
-- the contents have to be to match the expected contents. (0.0 = perfection,
-- 1.0 = very loose).  Note that Match_Threshold controls how closely the
-- end points of a delete need to match.
local Patch_DeleteThreshold = 0.5
-- Chunk size for context length.
local Patch_Margin = 4
-- The number of bits in an int.
local Match_MaxBits = 32

function settings(new)
  if new then
    Diff_Timeout = new.Diff_Timeout or Diff_Timeout
    Diff_EditCost = new.Diff_EditCost or Diff_EditCost
    Match_Threshold = new.Match_Threshold or Match_Threshold
    Match_Distance = new.Match_Distance or Match_Distance
    Patch_DeleteThreshold = new.Patch_DeleteThreshold or Patch_DeleteThreshold
    Patch_Margin = new.Patch_Margin or Patch_Margin
    Match_MaxBits = new.Match_MaxBits or Match_MaxBits
  else
    return {
      Diff_Timeout = Diff_Timeout;
      Diff_EditCost = Diff_EditCost;
      Match_Threshold = Match_Threshold;
      Match_Distance = Match_Distance;
      Patch_DeleteThreshold = Patch_DeleteThreshold;
      Patch_Margin = Patch_Margin;
      Match_MaxBits = Match_MaxBits;
    }
  end
end

-- ---------------------------------------------------------------------------
--  DIFF API
-- ---------------------------------------------------------------------------

-- The private diff functions
local _diff_compute,
      _diff_bisect,
      _diff_halfMatchI,
      _diff_halfMatch,
      _diff_cleanupSemanticScore,
      _diff_cleanupSemanticLossless,
      _diff_cleanupMerge,
      _diff_commonPrefix,
      _diff_commonSuffix,
      _diff_commonOverlap,
      _diff_xIndex,
      _diff_text1,
      _diff_text2,
      _diff_toDelta,
      _diff_fromDelta

--[[
* Find the differences between two texts.  Simplifies the problem by stripping
* any common prefix or suffix off the texts before diffing.
* @param {string} text1 Old string to be diffed.
* @param {string} text2 New string to be diffed.
* @param {boolean} opt_checklines Has no effect in Lua.
* @param {number} opt_deadline Optional time when the diff should be complete
*     by.  Used internally for recursive calls.  Users should set DiffTimeout
*     instead.
* @return {Array.<Array.<number|string>>} Array of diff tuples.
--]]
function diff_main(text1, text2, opt_checklines, opt_deadline)
  -- Set a deadline by which time the diff must be complete.
  if opt_deadline == nil then
    if Diff_Timeout <= 0 then
      opt_deadline = 2 ^ 31
    else
      opt_deadline = clock() + Diff_Timeout
    end
  end
  local deadline = opt_deadline

  -- Check for null inputs.
  if text1 == nil or text1 == nil then
    error('Null inputs. (diff_main)')
  end

  -- Check for equality (speedup).
  if text1 == text2 then
    if #text1 > 0 then
      return {{DIFF_EQUAL, text1}}
    end
    return {}
  end

  -- LUANOTE: Due to the lack of Unicode support, Lua is incapable of
  -- implementing the line-mode speedup.
  local checklines = false

  -- Trim off common prefix (speedup).
  local commonlength = _diff_commonPrefix(text1, text2)
  local commonprefix
  if commonlength > 0 then
    commonprefix = strsub(text1, 1, commonlength)
    text1 = strsub(text1, commonlength + 1)
    text2 = strsub(text2, commonlength + 1)
  end

  -- Trim off common suffix (speedup).
  commonlength = _diff_commonSuffix(text1, text2)
  local commonsuffix
  if commonlength > 0 then
    commonsuffix = strsub(text1, -commonlength)
    text1 = strsub(text1, 1, -commonlength - 1)
    text2 = strsub(text2, 1, -commonlength - 1)
  end

  -- Compute the diff on the middle block.
  local diffs = _diff_compute(text1, text2, checklines, deadline)

  -- Restore the prefix and suffix.
  if commonprefix then
    tinsert(diffs, 1, {DIFF_EQUAL, commonprefix})
  end
  if commonsuffix then
    diffs[#diffs + 1] = {DIFF_EQUAL, commonsuffix}
  end

  _diff_cleanupMerge(diffs)
  return diffs
end

--[[
* Reduce the number of edits by eliminating semantically trivial equalities.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
--]]
function diff_cleanupSemantic(diffs)
  local changes = false
  local equalities = {}  -- Stack of indices where equalities are found.
  local equalitiesLength = 0  -- Keeping our own length var is faster.
  local lastEquality = nil
  -- Always equal to diffs[equalities[equalitiesLength]][2]
  local pointer = 1  -- Index of current position.
  -- Number of characters that changed prior to the equality.
  local length_insertions1 = 0
  local length_deletions1 = 0
  -- Number of characters that changed after the equality.
  local length_insertions2 = 0
  local length_deletions2 = 0

  while diffs[pointer] do
    if diffs[pointer][1] == DIFF_EQUAL then  -- Equality found.
      equalitiesLength = equalitiesLength + 1
      equalities[equalitiesLength] = pointer
      length_insertions1 = length_insertions2
      length_deletions1 = length_deletions2
      length_insertions2 = 0
      length_deletions2 = 0
      lastEquality = diffs[pointer][2]
    else  -- An insertion or deletion.
      if diffs[pointer][1] == DIFF_INSERT then
        length_insertions2 = length_insertions2 + #(diffs[pointer][2])
      else
        length_deletions2 = length_deletions2 + #(diffs[pointer][2])
      end
      -- Eliminate an equality that is smaller or equal to the edits on both
      -- sides of it.
      if lastEquality
          and (#lastEquality <= max(length_insertions1, length_deletions1))
          and (#lastEquality <= max(length_insertions2, length_deletions2)) then
        -- Duplicate record.
        tinsert(diffs, equalities[equalitiesLength],
         {DIFF_DELETE, lastEquality})
        -- Change second copy to insert.
        diffs[equalities[equalitiesLength] + 1][1] = DIFF_INSERT
        -- Throw away the equality we just deleted.
        equalitiesLength = equalitiesLength - 1
        -- Throw away the previous equality (it needs to be reevaluated).
        equalitiesLength = equalitiesLength - 1
        pointer = (equalitiesLength > 0) and equalities[equalitiesLength] or 0
        length_insertions1, length_deletions1 = 0, 0  -- Reset the counters.
        length_insertions2, length_deletions2 = 0, 0
        lastEquality = nil
        changes = true
      end
    end
    pointer = pointer + 1
  end

  -- Normalize the diff.
  if changes then
    _diff_cleanupMerge(diffs)
  end
  _diff_cleanupSemanticLossless(diffs)

  -- Find any overlaps between deletions and insertions.
  -- e.g: <del>abcxxx</del><ins>xxxdef</ins>
  --   -> <del>abc</del>xxx<ins>def</ins>
  -- e.g: <del>xxxabc</del><ins>defxxx</ins>
  --   -> <ins>def</ins>xxx<del>abc</del>
  -- Only extract an overlap if it is as big as the edit ahead or behind it.
  pointer = 2
  while diffs[pointer] do
    if (diffs[pointer - 1][1] == DIFF_DELETE and
        diffs[pointer][1] == DIFF_INSERT) then
      local deletion = diffs[pointer - 1][2]
      local insertion = diffs[pointer][2]
      local overlap_length1 = _diff_commonOverlap(deletion, insertion)
      local overlap_length2 = _diff_commonOverlap(insertion, deletion)
      if (overlap_length1 >= overlap_length2) then
        if (overlap_length1 >= #deletion / 2 or
            overlap_length1 >= #insertion / 2) then
          -- Overlap found.  Insert an equality and trim the surrounding edits.
          tinsert(diffs, pointer,
              {DIFF_EQUAL, strsub(insertion, 1, overlap_length1)})
          diffs[pointer - 1][2] =
              strsub(deletion, 1, #deletion - overlap_length1)
          diffs[pointer + 1][2] = strsub(insertion, overlap_length1 + 1)
          pointer = pointer + 1
        end
      else
        if (overlap_length2 >= #deletion / 2 or
            overlap_length2 >= #insertion / 2) then
          -- Reverse overlap found.
          -- Insert an equality and swap and trim the surrounding edits.
          tinsert(diffs, pointer,
              {DIFF_EQUAL, strsub(deletion, 1, overlap_length2)})
          diffs[pointer - 1] = {DIFF_INSERT,
              strsub(insertion, 1, #insertion - overlap_length2)}
          diffs[pointer + 1] = {DIFF_DELETE,
              strsub(deletion, overlap_length2 + 1)}
          pointer = pointer + 1
        end
      end
      pointer = pointer + 1
    end
    pointer = pointer + 1
  end
end

--[[
* Reduce the number of edits by eliminating operationally trivial equalities.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
--]]
function diff_cleanupEfficiency(diffs)
  local changes = false
  -- Stack of indices where equalities are found.
  local equalities = {}
  -- Keeping our own length var is faster.
  local equalitiesLength = 0
  -- Always equal to diffs[equalities[equalitiesLength]][2]
  local lastEquality = nil
  -- Index of current position.
  local pointer = 1

  -- The following four are really booleans but are stored as numbers because
  -- they are used at one point like this:
  --
  -- (pre_ins + pre_del + post_ins + post_del) == 3
  --
  -- ...i.e. checking that 3 of them are true and 1 of them is false.

  -- Is there an insertion operation before the last equality.
  local pre_ins = 0
  -- Is there a deletion operation before the last equality.
  local pre_del = 0
  -- Is there an insertion operation after the last equality.
  local post_ins = 0
  -- Is there a deletion operation after the last equality.
  local post_del = 0

  while diffs[pointer] do
    if diffs[pointer][1] == DIFF_EQUAL then  -- Equality found.
      local diffText = diffs[pointer][2]
      if (#diffText < Diff_EditCost) and (post_ins == 1 or post_del == 1) then
        -- Candidate found.
        equalitiesLength = equalitiesLength + 1
        equalities[equalitiesLength] = pointer
        pre_ins, pre_del = post_ins, post_del
        lastEquality = diffText
      else
        -- Not a candidate, and can never become one.
        equalitiesLength = 0
        lastEquality = nil
      end
      post_ins, post_del = 0, 0
    else  -- An insertion or deletion.
      if diffs[pointer][1] == DIFF_DELETE then
        post_del = 1
      else
        post_ins = 1
      end
      --[[
      * Five types to be split:
      * <ins>A</ins><del>B</del>XY<ins>C</ins><del>D</del>
      * <ins>A</ins>X<ins>C</ins><del>D</del>
      * <ins>A</ins><del>B</del>X<ins>C</ins>
      * <ins>A</del>X<ins>C</ins><del>D</del>
      * <ins>A</ins><del>B</del>X<del>C</del>
      --]]
      if lastEquality and (
          (pre_ins+pre_del+post_ins+post_del == 4)
          or
          (
            (#lastEquality < Diff_EditCost / 2)
            and
            (pre_ins+pre_del+post_ins+post_del == 3)
          )) then
        -- Duplicate record.
        tinsert(diffs, equalities[equalitiesLength],
            {DIFF_DELETE, lastEquality})
        -- Change second copy to insert.
        diffs[equalities[equalitiesLength] + 1][1] = DIFF_INSERT
        -- Throw away the equality we just deleted.
        equalitiesLength = equalitiesLength - 1
        lastEquality = nil
        if (pre_ins == 1) and (pre_del == 1) then
          -- No changes made which could affect previous entry, keep going.
          post_ins, post_del = 1, 1
          equalitiesLength = 0
        else
          -- Throw away the previous equality.
          equalitiesLength = equalitiesLength - 1
          pointer = (equalitiesLength > 0) and equalities[equalitiesLength] or 0
          post_ins, post_del = 0, 0
        end
        changes = true
      end
    end
    pointer = pointer + 1
  end

  if changes then
    _diff_cleanupMerge(diffs)
  end
end

--[[
* Compute the Levenshtein distance; the number of inserted, deleted or
* substituted characters.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {number} Number of changes.
--]]
function diff_levenshtein(diffs)
  local levenshtein = 0
  local insertions, deletions = 0, 0
  for x, diff in ipairs(diffs) do
    local op, data = diff[1], diff[2]
    if (op == DIFF_INSERT) then
      insertions = insertions + #data
    elseif (op == DIFF_DELETE) then
      deletions = deletions + #data
    elseif (op == DIFF_EQUAL) then
      -- A deletion and an insertion is one substitution.
      levenshtein = levenshtein + max(insertions, deletions)
      insertions = 0
      deletions = 0
    end
  end
  levenshtein = levenshtein + max(insertions, deletions)
  return levenshtein
end

--[[
* Convert a diff array into a pretty HTML report.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {string} HTML representation.
--]]
function diff_prettyHtml(diffs)
  local html = {}
  for x, diff in ipairs(diffs) do
    local op = diff[1]   -- Operation (insert, delete, equal)
    local data = diff[2]  -- Text of change.
    local text = gsub(data, htmlEncode_pattern, htmlEncode_replace)
    if op == DIFF_INSERT then
      html[x] = '<ins style="background:#e6ffe6;">' .. text .. '</ins>'
    elseif op == DIFF_DELETE then
      html[x] = '<del style="background:#ffe6e6;">' .. text .. '</del>'
    elseif op == DIFF_EQUAL then
      html[x] = '<span>' .. text .. '</span>'
    end
  end
  return tconcat(html)
end

-- ---------------------------------------------------------------------------
-- UNOFFICIAL/PRIVATE DIFF FUNCTIONS
-- ---------------------------------------------------------------------------

--[[
* Find the differences between two texts.  Assumes that the texts do not
* have any common prefix or suffix.
* @param {string} text1 Old string to be diffed.
* @param {string} text2 New string to be diffed.
* @param {boolean} checklines Has no effect in Lua.
* @param {number} deadline Time when the diff should be complete by.
* @return {Array.<Array.<number|string>>} Array of diff tuples.
* @private
--]]
function _diff_compute(text1, text2, checklines, deadline)
  if #text1 == 0 then
    -- Just add some text (speedup).
    return {{DIFF_INSERT, text2}}
  end

  if #text2 == 0 then
    -- Just delete some text (speedup).
    return {{DIFF_DELETE, text1}}
  end

  local diffs

  local longtext = (#text1 > #text2) and text1 or text2
  local shorttext = (#text1 > #text2) and text2 or text1
  local i = indexOf(longtext, shorttext)

  if i ~= nil then
    -- Shorter text is inside the longer text (speedup).
    diffs = {
      {DIFF_INSERT, strsub(longtext, 1, i - 1)},
      {DIFF_EQUAL, shorttext},
      {DIFF_INSERT, strsub(longtext, i + #shorttext)}
    }
    -- Swap insertions for deletions if diff is reversed.
    if #text1 > #text2 then
      diffs[1][1], diffs[3][1] = DIFF_DELETE, DIFF_DELETE
    end
    return diffs
  end

  if #shorttext == 1 then
    -- Single character string.
    -- After the previous speedup, the character can't be an equality.
    return {{DIFF_DELETE, text1}, {DIFF_INSERT, text2}}
  end

  -- Check to see if the problem can be split in two.
  do
    local
     text1_a, text1_b,
     text2_a, text2_b,
     mid_common        = _diff_halfMatch(text1, text2)

    if text1_a then
      -- A half-match was found, sort out the return data.
      -- Send both pairs off for separate processing.
      local diffs_a = diff_main(text1_a, text2_a, checklines, deadline)
      local diffs_b = diff_main(text1_b, text2_b, checklines, deadline)
      -- Merge the results.
      local diffs_a_len = #diffs_a
      diffs = diffs_a
      diffs[diffs_a_len + 1] = {DIFF_EQUAL, mid_common}
      for i, b_diff in ipairs(diffs_b) do
        diffs[diffs_a_len + 1 + i] = b_diff
      end
      return diffs
    end
  end

  return _diff_bisect(text1, text2, deadline)
end

--[[
* Find the 'middle snake' of a diff, split the problem in two
* and return the recursively constructed diff.
* See Myers 1986 paper: An O(ND) Difference Algorithm and Its Variations.
* @param {string} text1 Old string to be diffed.
* @param {string} text2 New string to be diffed.
* @param {number} deadline Time at which to bail if not yet complete.
* @return {Array.<Array.<number|string>>} Array of diff tuples.
* @private
--]]
function _diff_bisect(text1, text2, deadline)
  -- Cache the text lengths to prevent multiple calls.
  local text1_length = #text1
  local text2_length = #text2
  local _sub, _element
  local max_d = ceil((text1_length + text2_length) / 2)
  local v_offset = max_d
  local v_length = 2 * max_d
  local v1 = {}
  local v2 = {}
  -- Setting all elements to -1 is faster in Lua than mixing integers and nil.
  for x = 0, v_length - 1 do
    v1[x] = -1
    v2[x] = -1
  end
  v1[v_offset + 1] = 0
  v2[v_offset + 1] = 0
  local delta = text1_length - text2_length
  -- If the total number of characters is odd, then
  -- the front path will collide with the reverse path.
  local front = (delta % 2 ~= 0)
  -- Offsets for start and end of k loop.
  -- Prevents mapping of space beyond the grid.
  local k1start = 0
  local k1end = 0
  local k2start = 0
  local k2end = 0
  for d = 0, max_d - 1 do
    -- Bail out if deadline is reached.
    if clock() > deadline then
      break
    end

    -- Walk the front path one step.
    for k1 = -d + k1start, d - k1end, 2 do
      local k1_offset = v_offset + k1
      local x1
      if (k1 == -d) or ((k1 ~= d) and
          (v1[k1_offset - 1] < v1[k1_offset + 1])) then
        x1 = v1[k1_offset + 1]
      else
        x1 = v1[k1_offset - 1] + 1
      end
      local y1 = x1 - k1
      while (x1 <= text1_length) and (y1 <= text2_length)
          and (strsub(text1, x1, x1) == strsub(text2, y1, y1)) do
        x1 = x1 + 1
        y1 = y1 + 1
      end
      v1[k1_offset] = x1
      if x1 > text1_length + 1 then
        -- Ran off the right of the graph.
        k1end = k1end + 2
      elseif y1 > text2_length + 1 then
        -- Ran off the bottom of the graph.
        k1start = k1start + 2
      elseif front then
        local k2_offset = v_offset + delta - k1
        if k2_offset >= 0 and k2_offset < v_length and v2[k2_offset] ~= -1 then
          -- Mirror x2 onto top-left coordinate system.
          local x2 = text1_length - v2[k2_offset] + 1
          if x1 > x2 then
            -- Overlap detected.
            return _diff_bisectSplit(text1, text2, x1, y1, deadline)
          end
        end
      end
    end

    -- Walk the reverse path one step.
    for k2 = -d + k2start, d - k2end, 2 do
      local k2_offset = v_offset + k2
      local x2
      if (k2 == -d) or ((k2 ~= d) and
          (v2[k2_offset - 1] < v2[k2_offset + 1])) then
        x2 = v2[k2_offset + 1]
      else
        x2 = v2[k2_offset - 1] + 1
      end
      local y2 = x2 - k2
      while (x2 <= text1_length) and (y2 <= text2_length)
          and (strsub(text1, -x2, -x2) == strsub(text2, -y2, -y2)) do
        x2 = x2 + 1
        y2 = y2 + 1
      end
      v2[k2_offset] = x2
      if x2 > text1_length + 1 then
        -- Ran off the left of the graph.
        k2end = k2end + 2
      elseif y2 > text2_length + 1 then
        -- Ran off the top of the graph.
        k2start = k2start + 2
      elseif not front then
        local k1_offset = v_offset + delta - k2
        if k1_offset >= 0 and k1_offset < v_length and v1[k1_offset] ~= -1 then
          local x1 = v1[k1_offset]
          local y1 = v_offset + x1 - k1_offset
          -- Mirror x2 onto top-left coordinate system.
          x2 = text1_length - x2 + 1
          if x1 > x2 then
            -- Overlap detected.
            return _diff_bisectSplit(text1, text2, x1, y1, deadline)
          end
        end
      end
    end
  end
  -- Diff took too long and hit the deadline or
  -- number of diffs equals number of characters, no commonality at all.
  return {{DIFF_DELETE, text1}, {DIFF_INSERT, text2}}
end

--[[
 * Given the location of the 'middle snake', split the diff in two parts
 * and recurse.
 * @param {string} text1 Old string to be diffed.
 * @param {string} text2 New string to be diffed.
 * @param {number} x Index of split point in text1.
 * @param {number} y Index of split point in text2.
 * @param {number} deadline Time at which to bail if not yet complete.
 * @return {Array.<Array.<number|string>>} Array of diff tuples.
 * @private
--]]
function _diff_bisectSplit(text1, text2, x, y, deadline)
  local text1a = strsub(text1, 1, x - 1)
  local text2a = strsub(text2, 1, y - 1)
  local text1b = strsub(text1, x)
  local text2b = strsub(text2, y)

  -- Compute both diffs serially.
  local diffs = diff_main(text1a, text2a, false, deadline)
  local diffsb = diff_main(text1b, text2b, false, deadline)

  local diffs_len = #diffs
  for i, v in ipairs(diffsb) do
    diffs[diffs_len + i] = v
  end
  return diffs
end

--[[
* Determine the common prefix of two strings.
* @param {string} text1 First string.
* @param {string} text2 Second string.
* @return {number} The number of characters common to the start of each
*    string.
--]]
function _diff_commonPrefix(text1, text2)
  -- Quick check for common null cases.
  if (#text1 == 0) or (#text2 == 0) or (strbyte(text1, 1) ~= strbyte(text2, 1))
      then
    return 0
  end
  -- Binary search.
  -- Performance analysis: https://neil.fraser.name/news/2007/10/09/
  local pointermin = 1
  local pointermax = min(#text1, #text2)
  local pointermid = pointermax
  local pointerstart = 1
  while (pointermin < pointermid) do
    if (strsub(text1, pointerstart, pointermid)
        == strsub(text2, pointerstart, pointermid)) then
      pointermin = pointermid
      pointerstart = pointermin
    else
      pointermax = pointermid
    end
    pointermid = floor(pointermin + (pointermax - pointermin) / 2)
  end
  return pointermid
end

--[[
* Determine the common suffix of two strings.
* @param {string} text1 First string.
* @param {string} text2 Second string.
* @return {number} The number of characters common to the end of each string.
--]]
function _diff_commonSuffix(text1, text2)
  -- Quick check for common null cases.
  if (#text1 == 0) or (#text2 == 0)
      or (strbyte(text1, -1) ~= strbyte(text2, -1)) then
    return 0
  end
  -- Binary search.
  -- Performance analysis: https://neil.fraser.name/news/2007/10/09/
  local pointermin = 1
  local pointermax = min(#text1, #text2)
  local pointermid = pointermax
  local pointerend = 1
  while (pointermin < pointermid) do
    if (strsub(text1, -pointermid, -pointerend)
        == strsub(text2, -pointermid, -pointerend)) then
      pointermin = pointermid
      pointerend = pointermin
    else
      pointermax = pointermid
    end
    pointermid = floor(pointermin + (pointermax - pointermin) / 2)
  end
  return pointermid
end

--[[
* Determine if the suffix of one string is the prefix of another.
* @param {string} text1 First string.
* @param {string} text2 Second string.
* @return {number} The number of characters common to the end of the first
*     string and the start of the second string.
* @private
--]]
function _diff_commonOverlap(text1, text2)
  -- Cache the text lengths to prevent multiple calls.
  local text1_length = #text1
  local text2_length = #text2
  -- Eliminate the null case.
  if text1_length == 0 or text2_length == 0 then
    return 0
  end
  -- Truncate the longer string.
  if text1_length > text2_length then
    text1 = strsub(text1, text1_length - text2_length + 1)
  elseif text1_length < text2_length then
    text2 = strsub(text2, 1, text1_length)
  end
  local text_length = min(text1_length, text2_length)
  -- Quick check for the worst case.
  if text1 == text2 then
    return text_length
  end

  -- Start by looking for a single character match
  -- and increase length until no match is found.
  -- Performance analysis: https://neil.fraser.name/news/2010/11/04/
  local best = 0
  local length = 1
  while true do
    local pattern = strsub(text1, text_length - length + 1)
    local found = strfind(text2, pattern, 1, true)
    if found == nil then
      return best
    end
    length = length + found - 1
    if found == 1 or strsub(text1, text_length - length + 1) ==
                     strsub(text2, 1, length) then
      best = length
      length = length + 1
    end
  end
end

--[[
* Does a substring of shorttext exist within longtext such that the substring
* is at least half the length of longtext?
* This speedup can produce non-minimal diffs.
* Closure, but does not reference any external variables.
* @param {string} longtext Longer string.
* @param {string} shorttext Shorter string.
* @param {number} i Start index of quarter length substring within longtext.
* @return {?Array.<string>} Five element Array, containing the prefix of
*    longtext, the suffix of longtext, the prefix of shorttext, the suffix
*    of shorttext and the common middle.  Or nil if there was no match.
* @private
--]]
function _diff_halfMatchI(longtext, shorttext, i)
  -- Start with a 1/4 length substring at position i as a seed.
  local seed = strsub(longtext, i, i + floor(#longtext / 4))
  local j = 0  -- LUANOTE: do not change to 1, was originally -1
  local best_common = ''
  local best_longtext_a, best_longtext_b, best_shorttext_a, best_shorttext_b
  while true do
    j = indexOf(shorttext, seed, j + 1)
    if (j == nil) then
      break
    end
    local prefixLength = _diff_commonPrefix(strsub(longtext, i),
        strsub(shorttext, j))
    local suffixLength = _diff_commonSuffix(strsub(longtext, 1, i - 1),
        strsub(shorttext, 1, j - 1))
    if #best_common < suffixLength + prefixLength then
      best_common = strsub(shorttext, j - suffixLength, j - 1)
          .. strsub(shorttext, j, j + prefixLength - 1)
      best_longtext_a = strsub(longtext, 1, i - suffixLength - 1)
      best_longtext_b = strsub(longtext, i + prefixLength)
      best_shorttext_a = strsub(shorttext, 1, j - suffixLength - 1)
      best_shorttext_b = strsub(shorttext, j + prefixLength)
    end
  end
  if #best_common * 2 >= #longtext then
    return {best_longtext_a, best_longtext_b,
            best_shorttext_a, best_shorttext_b, best_common}
  else
    return nil
  end
end

--[[
* Do the two texts share a substring which is at least half the length of the
* longer text?
* @param {string} text1 First string.
* @param {string} text2 Second string.
* @return {?Array.<string>} Five element Array, containing the prefix of
*    text1, the suffix of text1, the prefix of text2, the suffix of
*    text2 and the common middle.  Or nil if there was no match.
* @private
--]]
function _diff_halfMatch(text1, text2)
  if Diff_Timeout <= 0 then
    -- Don't risk returning a non-optimal diff if we have unlimited time.
    return nil
  end
  local longtext = (#text1 > #text2) and text1 or text2
  local shorttext = (#text1 > #text2) and text2 or text1
  if (#longtext < 4) or (#shorttext * 2 < #longtext) then
    return nil  -- Pointless.
  end

  -- First check if the second quarter is the seed for a half-match.
  local hm1 = _diff_halfMatchI(longtext, shorttext, ceil(#longtext / 4))
  -- Check again based on the third quarter.
  local hm2 = _diff_halfMatchI(longtext, shorttext, ceil(#longtext / 2))
  local hm
  if not hm1 and not hm2 then
    return nil
  elseif not hm2 then
    hm = hm1
  elseif not hm1 then
    hm = hm2
  else
    -- Both matched.  Select the longest.
    hm = (#hm1[5] > #hm2[5]) and hm1 or hm2
  end

  -- A half-match was found, sort out the return data.
  local text1_a, text1_b, text2_a, text2_b
  if (#text1 > #text2) then
    text1_a, text1_b = hm[1], hm[2]
    text2_a, text2_b = hm[3], hm[4]
  else
    text2_a, text2_b = hm[1], hm[2]
    text1_a, text1_b = hm[3], hm[4]
  end
  local mid_common = hm[5]
  return text1_a, text1_b, text2_a, text2_b, mid_common
end

--[[
* Given two strings, compute a score representing whether the internal
* boundary falls on logical boundaries.
* Scores range from 6 (best) to 0 (worst).
* @param {string} one First string.
* @param {string} two Second string.
* @return {number} The score.
* @private
--]]
function _diff_cleanupSemanticScore(one, two)
  if (#one == 0) or (#two == 0) then
    -- Edges are the best.
    return 6
  end

  -- Each port of this function behaves slightly differently due to
  -- subtle differences in each language's definition of things like
  -- 'whitespace'.  Since this function's purpose is largely cosmetic,
  -- the choice has been made to use each language's native features
  -- rather than force total conformity.
  local char1 = strsub(one, -1)
  local char2 = strsub(two, 1, 1)
  local nonAlphaNumeric1 = strmatch(char1, '%W')
  local nonAlphaNumeric2 = strmatch(char2, '%W')
  local whitespace1 = nonAlphaNumeric1 and strmatch(char1, '%s')
  local whitespace2 = nonAlphaNumeric2 and strmatch(char2, '%s')
  local lineBreak1 = whitespace1 and strmatch(char1, '%c')
  local lineBreak2 = whitespace2 and strmatch(char2, '%c')
  local blankLine1 = lineBreak1 and strmatch(one, '\n\r?\n$')
  local blankLine2 = lineBreak2 and strmatch(two, '^\r?\n\r?\n')

  if blankLine1 or blankLine2 then
    -- Five points for blank lines.
    return 5
  elseif lineBreak1 or lineBreak2 then
    -- Four points for line breaks.
    return 4
  elseif nonAlphaNumeric1 and not whitespace1 and whitespace2 then
    -- Three points for end of sentences.
    return 3
  elseif whitespace1 or whitespace2 then
    -- Two points for whitespace.
    return 2
  elseif nonAlphaNumeric1 or nonAlphaNumeric2 then
    -- One point for non-alphanumeric.
    return 1
  end
  return 0
end

--[[
* Look for single edits surrounded on both sides by equalities
* which can be shifted sideways to align the edit to a word boundary.
* e.g: The c<ins>at c</ins>ame. -> The <ins>cat </ins>came.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
--]]
function _diff_cleanupSemanticLossless(diffs)
  local pointer = 2
  -- Intentionally ignore the first and last element (don't need checking).
  while diffs[pointer + 1] do
    local prevDiff, nextDiff = diffs[pointer - 1], diffs[pointer + 1]
    if (prevDiff[1] == DIFF_EQUAL) and (nextDiff[1] == DIFF_EQUAL) then
      -- This is a single edit surrounded by equalities.
      local diff = diffs[pointer]

      local equality1 = prevDiff[2]
      local edit = diff[2]
      local equality2 = nextDiff[2]

      -- First, shift the edit as far left as possible.
      local commonOffset = _diff_commonSuffix(equality1, edit)
      if commonOffset > 0 then
        local commonString = strsub(edit, -commonOffset)
        equality1 = strsub(equality1, 1, -commonOffset - 1)
        edit = commonString .. strsub(edit, 1, -commonOffset - 1)
        equality2 = commonString .. equality2
      end

      -- Second, step character by character right, looking for the best fit.
      local bestEquality1 = equality1
      local bestEdit = edit
      local bestEquality2 = equality2
      local bestScore = _diff_cleanupSemanticScore(equality1, edit)
          + _diff_cleanupSemanticScore(edit, equality2)

      while strbyte(edit, 1) == strbyte(equality2, 1) do
        equality1 = equality1 .. strsub(edit, 1, 1)
        edit = strsub(edit, 2) .. strsub(equality2, 1, 1)
        equality2 = strsub(equality2, 2)
        local score = _diff_cleanupSemanticScore(equality1, edit)
            + _diff_cleanupSemanticScore(edit, equality2)
        -- The >= encourages trailing rather than leading whitespace on edits.
        if score >= bestScore then
          bestScore = score
          bestEquality1 = equality1
          bestEdit = edit
          bestEquality2 = equality2
        end
      end
      if prevDiff[2] ~= bestEquality1 then
        -- We have an improvement, save it back to the diff.
        if #bestEquality1 > 0 then
          diffs[pointer - 1][2] = bestEquality1
        else
          tremove(diffs, pointer - 1)
          pointer = pointer - 1
        end
        diffs[pointer][2] = bestEdit
        if #bestEquality2 > 0 then
          diffs[pointer + 1][2] = bestEquality2
        else
          tremove(diffs, pointer + 1, 1)
          pointer = pointer - 1
        end
      end
    end
    pointer = pointer + 1
  end
end

--[[
* Reorder and merge like edit sections.  Merge equalities.
* Any edit section can move as long as it doesn't cross an equality.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
--]]
function _diff_cleanupMerge(diffs)
  diffs[#diffs + 1] = {DIFF_EQUAL, ''}  -- Add a dummy entry at the end.
  local pointer = 1
  local count_delete, count_insert = 0, 0
  local text_delete, text_insert = '', ''
  local commonlength
  while diffs[pointer] do
    local diff_type = diffs[pointer][1]
    if diff_type == DIFF_INSERT then
      count_insert = count_insert + 1
      text_insert = text_insert .. diffs[pointer][2]
      pointer = pointer + 1
    elseif diff_type == DIFF_DELETE then
      count_delete = count_delete + 1
      text_delete = text_delete .. diffs[pointer][2]
      pointer = pointer + 1
    elseif diff_type == DIFF_EQUAL then
      -- Upon reaching an equality, check for prior redundancies.
      if count_delete + count_insert > 1 then
        if (count_delete > 0) and (count_insert > 0) then
          -- Factor out any common prefixies.
          commonlength = _diff_commonPrefix(text_insert, text_delete)
          if commonlength > 0 then
            local back_pointer = pointer - count_delete - count_insert
            if (back_pointer > 1) and (diffs[back_pointer - 1][1] == DIFF_EQUAL)
                then
              diffs[back_pointer - 1][2] = diffs[back_pointer - 1][2]
                  .. strsub(text_insert, 1, commonlength)
            else
              tinsert(diffs, 1,
                  {DIFF_EQUAL, strsub(text_insert, 1, commonlength)})
              pointer = pointer + 1
            end
            text_insert = strsub(text_insert, commonlength + 1)
            text_delete = strsub(text_delete, commonlength + 1)
          end
          -- Factor out any common suffixies.
          commonlength = _diff_commonSuffix(text_insert, text_delete)
          if commonlength ~= 0 then
            diffs[pointer][2] =
                strsub(text_insert, -commonlength) .. diffs[pointer][2]
            text_insert = strsub(text_insert, 1, -commonlength - 1)
            text_delete = strsub(text_delete, 1, -commonlength - 1)
          end
        end
        -- Delete the offending records and add the merged ones.
        pointer = pointer - count_delete - count_insert
        for i = 1, count_delete + count_insert do
          tremove(diffs, pointer)
        end
        if #text_delete > 0 then
          tinsert(diffs, pointer, {DIFF_DELETE, text_delete})
          pointer = pointer + 1
        end
        if #text_insert > 0 then
          tinsert(diffs, pointer, {DIFF_INSERT, text_insert})
          pointer = pointer + 1
        end
        pointer = pointer + 1
      elseif (pointer > 1) and (diffs[pointer - 1][1] == DIFF_EQUAL) then
        -- Merge this equality with the previous one.
        diffs[pointer - 1][2] = diffs[pointer - 1][2] .. diffs[pointer][2]
        tremove(diffs, pointer)
      else
        pointer = pointer + 1
      end
      count_insert, count_delete = 0, 0
      text_delete, text_insert = '', ''
    end
  end
  if diffs[#diffs][2] == '' then
    diffs[#diffs] = nil  -- Remove the dummy entry at the end.
  end

  -- Second pass: look for single edits surrounded on both sides by equalities
  -- which can be shifted sideways to eliminate an equality.
  -- e.g: A<ins>BA</ins>C -> <ins>AB</ins>AC
  local changes = false
  pointer = 2
  -- Intentionally ignore the first and last element (don't need checking).
  while pointer < #diffs do
    local prevDiff, nextDiff = diffs[pointer - 1], diffs[pointer + 1]
    if (prevDiff[1] == DIFF_EQUAL) and (nextDiff[1] == DIFF_EQUAL) then
      -- This is a single edit surrounded by equalities.
      local diff = diffs[pointer]
      local currentText = diff[2]
      local prevText = prevDiff[2]
      local nextText = nextDiff[2]
      if #prevText == 0 then
        tremove(diffs, pointer - 1)
        changes = true
      elseif strsub(currentText, -#prevText) == prevText then
        -- Shift the edit over the previous equality.
        diff[2] = prevText .. strsub(currentText, 1, -#prevText - 1)
        nextDiff[2] = prevText .. nextDiff[2]
        tremove(diffs, pointer - 1)
        changes = true
      elseif strsub(currentText, 1, #nextText) == nextText then
        -- Shift the edit over the next equality.
        prevDiff[2] = prevText .. nextText
        diff[2] = strsub(currentText, #nextText + 1) .. nextText
        tremove(diffs, pointer + 1)
        changes = true
      end
    end
    pointer = pointer + 1
  end
  -- If shifts were made, the diff needs reordering and another shift sweep.
  if changes then
    -- LUANOTE: no return value, but necessary to use 'return' to get
    -- tail calls.
    return _diff_cleanupMerge(diffs)
  end
end

--[[
* loc is a location in text1, compute and return the equivalent location in
* text2.
* e.g. 'The cat' vs 'The big cat', 1->1, 5->8
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @param {number} loc Location within text1.
* @return {number} Location within text2.
--]]
function _diff_xIndex(diffs, loc)
  local chars1 = 1
  local chars2 = 1
  local last_chars1 = 1
  local last_chars2 = 1
  local x
  for _x, diff in ipairs(diffs) do
    x = _x
    if diff[1] ~= DIFF_INSERT then   -- Equality or deletion.
      chars1 = chars1 + #diff[2]
    end
    if diff[1] ~= DIFF_DELETE then   -- Equality or insertion.
      chars2 = chars2 + #diff[2]
    end
    if chars1 > loc then   -- Overshot the location.
      break
    end
    last_chars1 = chars1
    last_chars2 = chars2
  end
  -- Was the location deleted?
  if diffs[x + 1] and (diffs[x][1] == DIFF_DELETE) then
    return last_chars2
  end
  -- Add the remaining character length.
  return last_chars2 + (loc - last_chars1)
end

--[[
* Compute and return the source text (all equalities and deletions).
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {string} Source text.
--]]
function _diff_text1(diffs)
  local text = {}
  for x, diff in ipairs(diffs) do
    if diff[1] ~= DIFF_INSERT then
      text[#text + 1] = diff[2]
    end
  end
  return tconcat(text)
end

--[[
* Compute and return the destination text (all equalities and insertions).
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {string} Destination text.
--]]
function _diff_text2(diffs)
  local text = {}
  for x, diff in ipairs(diffs) do
    if diff[1] ~= DIFF_DELETE then
      text[#text + 1] = diff[2]
    end
  end
  return tconcat(text)
end

--[[
* Crush the diff into an encoded string which describes the operations
* required to transform text1 into text2.
* E.g. =3\t-2\t+ing  -> Keep 3 chars, delete 2 chars, insert 'ing'.
* Operations are tab-separated.  Inserted text is escaped using %xx notation.
* @param {Array.<Array.<number|string>>} diffs Array of diff tuples.
* @return {string} Delta text.
--]]
function _diff_toDelta(diffs)
  local text = {}
  for x, diff in ipairs(diffs) do
    local op, data = diff[1], diff[2]
    if op == DIFF_INSERT then
      text[x] = '+' .. gsub(data, percentEncode_pattern, percentEncode_replace)
    elseif op == DIFF_DELETE then
      text[x] = '-' .. #data
    elseif op == DIFF_EQUAL then
      text[x] = '=' .. #data
    end
  end
  return tconcat(text, '\t')
end

--[[
* Given the original text1, and an encoded string which describes the
* operations required to transform text1 into text2, compute the full diff.
* @param {string} text1 Source string for the diff.
* @param {string} delta Delta text.
* @return {Array.<Array.<number|string>>} Array of diff tuples.
* @throws {Errorend If invalid input.
--]]
function _diff_fromDelta(text1, delta)
  local diffs = {}
  local diffsLength = 0  -- Keeping our own length var is faster
  local pointer = 1  -- Cursor in text1
  for token in gmatch(delta, '[^\t]+') do
    -- Each token begins with a one character parameter which specifies the
    -- operation of this token (delete, insert, equality).
    local tokenchar, param = strsub(token, 1, 1), strsub(token, 2)
    if (tokenchar == '+') then
      local invalidDecode = false
      local decoded = gsub(param, '%%(.?.?)',
          function(c)
            local n = tonumber(c, 16)
            if (#c ~= 2) or (n == nil) then
              invalidDecode = true
              return ''
            end
            return strchar(n)
          end)
      if invalidDecode then
        -- Malformed URI sequence.
        error('Illegal escape in _diff_fromDelta: ' .. param)
      end
      diffsLength = diffsLength + 1
      diffs[diffsLength] = {DIFF_INSERT, decoded}
    elseif (tokenchar == '-') or (tokenchar == '=') then
      local n = tonumber(param)
      if (n == nil) or (n < 0) then
        error('Invalid number in _diff_fromDelta: ' .. param)
      end
      local text = strsub(text1, pointer, pointer + n - 1)
      pointer = pointer + n
      if (tokenchar == '=') then
        diffsLength = diffsLength + 1
        diffs[diffsLength] = {DIFF_EQUAL, text}
      else
        diffsLength = diffsLength + 1
        diffs[diffsLength] = {DIFF_DELETE, text}
      end
    else
      error('Invalid diff operation in _diff_fromDelta: ' .. token)
    end
  end
  if (pointer ~= #text1 + 1) then
    error('Delta length (' .. (pointer - 1)
        .. ') does not equal source text length (' .. #text1 .. ').')
  end
  return diffs
end

-- ---------------------------------------------------------------------------
--  MATCH API
-- ---------------------------------------------------------------------------

local _match_bitap, _match_alphabet

--[[
* Locate the best instance of 'pattern' in 'text' near 'loc'.
* @param {string} text The text to search.
* @param {string} pattern The pattern to search for.
* @param {number} loc The location to search around.
* @return {number} Best match index or -1.
--]]
function match_main(text, pattern, loc)
  -- Check for null inputs.
  if text == nil or pattern == nil or loc == nil then
    error('Null inputs. (match_main)')
  end

  if text == pattern then
    -- Shortcut (potentially not guaranteed by the algorithm)
    return 1
  elseif #text == 0 then
    -- Nothing to match.
    return -1
  end
  loc = max(1, min(loc, #text))
  if strsub(text, loc, loc + #pattern - 1) == pattern then
    -- Perfect match at the perfect spot!  (Includes case of null pattern)
    return loc
  else
    -- Do a fuzzy compare.
    return _match_bitap(text, pattern, loc)
  end
end

-- ---------------------------------------------------------------------------
-- UNOFFICIAL/PRIVATE MATCH FUNCTIONS
-- ---------------------------------------------------------------------------

--[[
* Initialise the alphabet for the Bitap algorithm.
* @param {string} pattern The text to encode.
* @return {Object} Hash of character locations.
* @private
--]]
function _match_alphabet(pattern)
  local s = {}
  local i = 0
  for c in gmatch(pattern, '.') do
    s[c] = bor(s[c] or 0, lshift(1, #pattern - i - 1))
    i = i + 1
  end
  return s
end

--[[
* Locate the best instance of 'pattern' in 'text' near 'loc' using the
* Bitap algorithm.
* @param {string} text The text to search.
* @param {string} pattern The pattern to search for.
* @param {number} loc The location to search around.
* @return {number} Best match index or -1.
* @private
--]]
function _match_bitap(text, pattern, loc)
  if #pattern > Match_MaxBits then
    error('Pattern too long.')
  end

  -- Initialise the alphabet.
  local s = _match_alphabet(pattern)

  --[[
  * Compute and return the score for a match with e errors and x location.
  * Accesses loc and pattern through being a closure.
  * @param {number} e Number of errors in match.
  * @param {number} x Location of match.
  * @return {number} Overall score for match (0.0 = good, 1.0 = bad).
  * @private
  --]]
  local function _match_bitapScore(e, x)
    local accuracy = e / #pattern
    local proximity = abs(loc - x)
    if (Match_Distance == 0) then
      -- Dodge divide by zero error.
      return (proximity == 0) and 1 or accuracy
    end
    return accuracy + (proximity / Match_Distance)
  end

  -- Highest score beyond which we give up.
  local score_threshold = Match_Threshold
  -- Is there a nearby exact match? (speedup)
  local best_loc = indexOf(text, pattern, loc)
  if best_loc then
    score_threshold = min(_match_bitapScore(0, best_loc), score_threshold)
    -- LUANOTE: Ideally we'd also check from the other direction, but Lua
    -- doesn't have an efficent lastIndexOf function.
  end

  -- Initialise the bit arrays.
  local matchmask = lshift(1, #pattern - 1)
  best_loc = -1

  local bin_min, bin_mid
  local bin_max = #pattern + #text
  local last_rd
  for d = 0, #pattern - 1, 1 do
    -- Scan for the best match; each iteration allows for one more error.
    -- Run a binary search to determine how far from 'loc' we can stray at this
    -- error level.
    bin_min = 0
    bin_mid = bin_max
    while (bin_min < bin_mid) do
      if (_match_bitapScore(d, loc + bin_mid) <= score_threshold) then
        bin_min = bin_mid
      else
        bin_max = bin_mid
      end
      bin_mid = floor(bin_min + (bin_max - bin_min) / 2)
    end
    -- Use the result from this iteration as the maximum for the next.
    bin_max = bin_mid
    local start = max(1, loc - bin_mid + 1)
    local finish = min(loc + bin_mid, #text) + #pattern

    local rd = {}
    for j = start, finish do
      rd[j] = 0
    end
    rd[finish + 1] = lshift(1, d) - 1
    for j = finish, start, -1 do
      local charMatch = s[strsub(text, j - 1, j - 1)] or 0
      if (d == 0) then  -- First pass: exact match.
        rd[j] = band(bor((rd[j + 1] * 2), 1), charMatch)
      else
        -- Subsequent passes: fuzzy match.
        -- Functions instead of operators make this hella messy.
        rd[j] = bor(
                band(
                  bor(
                    lshift(rd[j + 1], 1),
                    1
                  ),
                  charMatch
                ),
                bor(
                  bor(
                    lshift(bor(last_rd[j + 1], last_rd[j]), 1),
                    1
                  ),
                  last_rd[j + 1]
                )
              )
      end
      if (band(rd[j], matchmask) ~= 0) then
        local score = _match_bitapScore(d, j - 1)
        -- This match will almost certainly be better than any existing match.
        -- But check anyway.
        if (score <= score_threshold) then
          -- Told you so.
          score_threshold = score
          best_loc = j - 1
          if (best_loc > loc) then
            -- When passing loc, don't exceed our current distance from loc.
            start = max(1, loc * 2 - best_loc)
          else
            -- Already passed loc, downhill from here on in.
            break
          end
        end
      end
    end
    -- No hope for a (better) match at greater error levels.
    if (_match_bitapScore(d + 1, loc) > score_threshold) then
      break
    end
    last_rd = rd
  end
  return best_loc
end

-- -----------------------------------------------------------------------------
-- PATCH API
-- -----------------------------------------------------------------------------

local _patch_addContext,
      _patch_deepCopy,
      _patch_addPadding,
      _patch_splitMax,
      _patch_appendText,
      _new_patch_obj

--[[
* Compute a list of patches to turn text1 into text2.
* Use diffs if provided, otherwise compute it ourselves.
* There are four ways to call this function, depending on what data is
* available to the caller:
* Method 1:
* a = text1, b = text2
* Method 2:
* a = diffs
* Method 3 (optimal):
* a = text1, b = diffs
* Method 4 (deprecated, use method 3):
* a = text1, b = text2, c = diffs
*
* @param {string|Array.<Array.<number|string>>} a text1 (methods 1,3,4) or
* Array of diff tuples for text1 to text2 (method 2).
* @param {string|Array.<Array.<number|string>>} opt_b text2 (methods 1,4) or
* Array of diff tuples for text1 to text2 (method 3) or undefined (method 2).
* @param {string|Array.<Array.<number|string>>} opt_c Array of diff tuples for
* text1 to text2 (method 4) or undefined (methods 1,2,3).
* @return {Array.<_new_patch_obj>} Array of patch objects.
--]]
function patch_make(a, opt_b, opt_c)
  local text1, diffs
  local type_a, type_b, type_c = type(a), type(opt_b), type(opt_c)
  if (type_a == 'string') and (type_b == 'string') and (type_c == 'nil') then
    -- Method 1: text1, text2
    -- Compute diffs from text1 and text2.
    text1 = a
    diffs = diff_main(text1, opt_b, true)
    if (#diffs > 2) then
      diff_cleanupSemantic(diffs)
      diff_cleanupEfficiency(diffs)
    end
  elseif (type_a == 'table') and (type_b == 'nil') and (type_c == 'nil') then
    -- Method 2: diffs
    -- Compute text1 from diffs.
    diffs = a
    text1 = _diff_text1(diffs)
  elseif (type_a == 'string') and (type_b == 'table') and (type_c == 'nil') then
    -- Method 3: text1, diffs
    text1 = a
    diffs = opt_b
  elseif (type_a == 'string') and (type_b == 'string') and (type_c == 'table')
      then
    -- Method 4: text1, text2, diffs
    -- text2 is not used.
    text1 = a
    diffs = opt_c
  else
    error('Unknown call format to patch_make.')
  end

  if (diffs[1] == nil) then
    return {}  -- Get rid of the null case.
  end

  local patches = {}
  local patch = _new_patch_obj()
  local patchDiffLength = 0  -- Keeping our own length var is faster.
  local char_count1 = 0  -- Number of characters into the text1 string.
  local char_count2 = 0  -- Number of characters into the text2 string.
  -- Start with text1 (prepatch_text) and apply the diffs until we arrive at
  -- text2 (postpatch_text).  We recreate the patches one by one to determine
  -- context info.
  local prepatch_text, postpatch_text = text1, text1
  for x, diff in ipairs(diffs) do
    local diff_type, diff_text = diff[1], diff[2]

    if (patchDiffLength == 0) and (diff_type ~= DIFF_EQUAL) then
      -- A new patch starts here.
      patch.start1 = char_count1 + 1
      patch.start2 = char_count2 + 1
    end

    if (diff_type == DIFF_INSERT) then
      patchDiffLength = patchDiffLength + 1
      patch.diffs[patchDiffLength] = diff
      patch.length2 = patch.length2 + #diff_text
      postpatch_text = strsub(postpatch_text, 1, char_count2)
          .. diff_text .. strsub(postpatch_text, char_count2 + 1)
    elseif (diff_type == DIFF_DELETE) then
      patch.length1 = patch.length1 + #diff_text
      patchDiffLength = patchDiffLength + 1
      patch.diffs[patchDiffLength] = diff
      postpatch_text = strsub(postpatch_text, 1, char_count2)
          .. strsub(postpatch_text, char_count2 + #diff_text + 1)
    elseif (diff_type == DIFF_EQUAL) then
      if (#diff_text <= Patch_Margin * 2)
          and (patchDiffLength ~= 0) and (#diffs ~= x) then
        -- Small equality inside a patch.
        patchDiffLength = patchDiffLength + 1
        patch.diffs[patchDiffLength] = diff
        patch.length1 = patch.length1 + #diff_text
        patch.length2 = patch.length2 + #diff_text
      elseif (#diff_text >= Patch_Margin * 2) then
        -- Time for a new patch.
        if (patchDiffLength ~= 0) then
          _patch_addContext(patch, prepatch_text)
          patches[#patches + 1] = patch
          patch = _new_patch_obj()
          patchDiffLength = 0
          -- Unlike Unidiff, our patch lists have a rolling context.
          -- https://github.com/google/diff-match-patch/wiki/Unidiff
          -- Update prepatch text & pos to reflect the application of the
          -- just completed patch.
          prepatch_text = postpatch_text
          char_count1 = char_count2
        end
      end
    end

    -- Update the current character count.
    if (diff_type ~= DIFF_INSERT) then
      char_count1 = char_count1 + #diff_text
    end
    if (diff_type ~= DIFF_DELETE) then
      char_count2 = char_count2 + #diff_text
    end
  end

  -- Pick up the leftover patch if not empty.
  if (patchDiffLength > 0) then
    _patch_addContext(patch, prepatch_text)
    patches[#patches + 1] = patch
  end

  return patches
end

--[[
* Merge a set of patches onto the text.  Return a patched text, as well
* as a list of true/false values indicating which patches were applied.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
* @param {string} text Old text.
* @return {Array.<string|Array.<boolean>>} Two return values, the
*     new text and an array of boolean values.
--]]
function patch_apply(patches, text)
  if patches[1] == nil then
    return text, {}
  end

  -- Deep copy the patches so that no changes are made to originals.
  patches = _patch_deepCopy(patches)

  local nullPadding = _patch_addPadding(patches)
  text = nullPadding .. text .. nullPadding

  _patch_splitMax(patches)
  -- delta keeps track of the offset between the expected and actual location
  -- of the previous patch. If there are patches expected at positions 10 and
  -- 20, but the first patch was found at 12, delta is 2 and the second patch
  -- has an effective expected position of 22.
  local delta = 0
  local results = {}
  for x, patch in ipairs(patches) do
    local expected_loc = patch.start2 + delta
    local text1 = _diff_text1(patch.diffs)
    local start_loc
    local end_loc = -1
    if #text1 > Match_MaxBits then
      -- _patch_splitMax will only provide an oversized pattern in
      -- the case of a monster delete.
      start_loc = match_main(text,
          strsub(text1, 1, Match_MaxBits), expected_loc)
      if start_loc ~= -1 then
        end_loc = match_main(text, strsub(text1, -Match_MaxBits),
            expected_loc + #text1 - Match_MaxBits)
        if end_loc == -1 or start_loc >= end_loc then
          -- Can't find valid trailing context.  Drop this patch.
          start_loc = -1
        end
      end
    else
      start_loc = match_main(text, text1, expected_loc)
    end
    if start_loc == -1 then
      -- No match found.  :(
      results[x] = false
      -- Subtract the delta for this failed patch from subsequent patches.
      delta = delta - patch.length2 - patch.length1
    else
      -- Found a match.  :)
      results[x] = true
      delta = start_loc - expected_loc
      local text2
      if end_loc == -1 then
        text2 = strsub(text, start_loc, start_loc + #text1 - 1)
      else
        text2 = strsub(text, start_loc, end_loc + Match_MaxBits - 1)
      end
      if text1 == text2 then
        -- Perfect match, just shove the replacement text in.
        text = strsub(text, 1, start_loc - 1) .. _diff_text2(patch.diffs)
            .. strsub(text, start_loc + #text1)
      else
        -- Imperfect match.  Run a diff to get a framework of equivalent
        -- indices.
        local diffs = diff_main(text1, text2, false)
        if (#text1 > Match_MaxBits)
            and (diff_levenshtein(diffs) / #text1 > Patch_DeleteThreshold) then
          -- The end points match, but the content is unacceptably bad.
          results[x] = false
        else
          _diff_cleanupSemanticLossless(diffs)
          local index1 = 1
          local index2
          for y, mod in ipairs(patch.diffs) do
            if mod[1] ~= DIFF_EQUAL then
              index2 = _diff_xIndex(diffs, index1)
            end
            if mod[1] == DIFF_INSERT then
              text = strsub(text, 1, start_loc + index2 - 2)
                  .. mod[2] .. strsub(text, start_loc + index2 - 1)
            elseif mod[1] == DIFF_DELETE then
              text = strsub(text, 1, start_loc + index2 - 2) .. strsub(text,
                  start_loc + _diff_xIndex(diffs, index1 + #mod[2] - 1))
            end
            if mod[1] ~= DIFF_DELETE then
              index1 = index1 + #mod[2]
            end
          end
        end
      end
    end
  end
  -- Strip the padding off.
  text = strsub(text, #nullPadding + 1, -#nullPadding - 1)
  return text, results
end

--[[
* Take a list of patches and return a textual representation.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
* @return {string} Text representation of patches.
--]]
function patch_toText(patches)
  local text = {}
  for x, patch in ipairs(patches) do
    _patch_appendText(patch, text)
  end
  return tconcat(text)
end

--[[
* Parse a textual representation of patches and return a list of patch objects.
* @param {string} textline Text representation of patches.
* @return {Array.<_new_patch_obj>} Array of patch objects.
* @throws {Error} If invalid input.
--]]
function patch_fromText(textline)
  local patches = {}
  if (#textline == 0) then
    return patches
  end
  local text = {}
  for line in gmatch(textline, '([^\n]*)') do
    text[#text + 1] = line
  end
  local textPointer = 1
  while (textPointer <= #text) do
    local start1, length1, start2, length2
     = strmatch(text[textPointer], '^@@ %-(%d+),?(%d*) %+(%d+),?(%d*) @@$')
    if (start1 == nil) then
      error('Invalid patch string: "' .. text[textPointer] .. '"')
    end
    local patch = _new_patch_obj()
    patches[#patches + 1] = patch

    start1 = tonumber(start1)
    length1 = tonumber(length1) or 1
    if (length1 == 0) then
      start1 = start1 + 1
    end
    patch.start1 = start1
    patch.length1 = length1

    start2 = tonumber(start2)
    length2 = tonumber(length2) or 1
    if (length2 == 0) then
      start2 = start2 + 1
    end
    patch.start2 = start2
    patch.length2 = length2

    textPointer = textPointer + 1

    while true do
      local line = text[textPointer]
      if (line == nil) then
        break
      end
      local sign; sign, line = strsub(line, 1, 1), strsub(line, 2)

      local invalidDecode = false
      local decoded = gsub(line, '%%(.?.?)',
          function(c)
            local n = tonumber(c, 16)
            if (#c ~= 2) or (n == nil) then
              invalidDecode = true
              return ''
            end
            return strchar(n)
          end)
      if invalidDecode then
        -- Malformed URI sequence.
        error('Illegal escape in patch_fromText: ' .. line)
      end

      line = decoded

      if (sign == '-') then
        -- Deletion.
        patch.diffs[#patch.diffs + 1] = {DIFF_DELETE, line}
      elseif (sign == '+') then
        -- Insertion.
        patch.diffs[#patch.diffs + 1] = {DIFF_INSERT, line}
      elseif (sign == ' ') then
        -- Minor equality.
        patch.diffs[#patch.diffs + 1] = {DIFF_EQUAL, line}
      elseif (sign == '@') then
        -- Start of next patch.
        break
      elseif (sign == '') then
        -- Blank line?  Whatever.
      else
        -- WTF?
        error('Invalid patch mode "' .. sign .. '" in: ' .. line)
      end
      textPointer = textPointer + 1
    end
  end
  return patches
end

-- ---------------------------------------------------------------------------
-- UNOFFICIAL/PRIVATE PATCH FUNCTIONS
-- ---------------------------------------------------------------------------

local patch_meta = {
  __tostring = function(patch)
    local buf = {}
    _patch_appendText(patch, buf)
    return tconcat(buf)
  end
}

--[[
* Class representing one patch operation.
* @constructor
--]]
function _new_patch_obj()
  return setmetatable({
    --[[ @type {Array.<Array.<number|string>>} ]]
    diffs = {};
    --[[ @type {?number} ]]
    start1 = 1;  -- nil;
    --[[ @type {?number} ]]
    start2 = 1;  -- nil;
    --[[ @type {number} ]]
    length1 = 0;
    --[[ @type {number} ]]
    length2 = 0;
  }, patch_meta)
end

--[[
* Increase the context until it is unique,
* but don't let the pattern expand beyond Match_MaxBits.
* @param {_new_patch_obj} patch The patch to grow.
* @param {string} text Source text.
* @private
--]]
function _patch_addContext(patch, text)
  if (#text == 0) then
    return
  end
  local pattern = strsub(text, patch.start2, patch.start2 + patch.length1 - 1)
  local padding = 0

  -- LUANOTE: Lua's lack of a lastIndexOf function results in slightly
  -- different logic here than in other language ports.
  -- Look for the first two matches of pattern in text.  If two are found,
  -- increase the pattern length.
  local firstMatch = indexOf(text, pattern)
  local secondMatch = nil
  if (firstMatch ~= nil) then
    secondMatch = indexOf(text, pattern, firstMatch + 1)
  end
  while (#pattern == 0 or secondMatch ~= nil)
      and (#pattern < Match_MaxBits - Patch_Margin - Patch_Margin) do
    padding = padding + Patch_Margin
    pattern = strsub(text, max(1, patch.start2 - padding),
    patch.start2 + patch.length1 - 1 + padding)
    firstMatch = indexOf(text, pattern)
    if (firstMatch ~= nil) then
      secondMatch = indexOf(text, pattern, firstMatch + 1)
    else
      secondMatch = nil
    end
  end
  -- Add one chunk for good luck.
  padding = padding + Patch_Margin

  -- Add the prefix.
  local prefix = strsub(text, max(1, patch.start2 - padding), patch.start2 - 1)
  if (#prefix > 0) then
    tinsert(patch.diffs, 1, {DIFF_EQUAL, prefix})
  end
  -- Add the suffix.
  local suffix = strsub(text, patch.start2 + patch.length1,
  patch.start2 + patch.length1 - 1 + padding)
  if (#suffix > 0) then
    patch.diffs[#patch.diffs + 1] = {DIFF_EQUAL, suffix}
  end

  -- Roll back the start points.
  patch.start1 = patch.start1 - #prefix
  patch.start2 = patch.start2 - #prefix
  -- Extend the lengths.
  patch.length1 = patch.length1 + #prefix + #suffix
  patch.length2 = patch.length2 + #prefix + #suffix
end

--[[
* Given an array of patches, return another array that is identical.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
* @return {Array.<_new_patch_obj>} Array of patch objects.
--]]
function _patch_deepCopy(patches)
  local patchesCopy = {}
  for x, patch in ipairs(patches) do
    local patchCopy = _new_patch_obj()
    local diffsCopy = {}
    for i, diff in ipairs(patch.diffs) do
      diffsCopy[i] = {diff[1], diff[2]}
    end
    patchCopy.diffs = diffsCopy
    patchCopy.start1 = patch.start1
    patchCopy.start2 = patch.start2
    patchCopy.length1 = patch.length1
    patchCopy.length2 = patch.length2
    patchesCopy[x] = patchCopy
  end
  return patchesCopy
end

--[[
* Add some padding on text start and end so that edges can match something.
* Intended to be called only from within patch_apply.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
* @return {string} The padding string added to each side.
--]]
function _patch_addPadding(patches)
  local paddingLength = Patch_Margin
  local nullPadding = ''
  for x = 1, paddingLength do
    nullPadding = nullPadding .. strchar(x)
  end

  -- Bump all the patches forward.
  for x, patch in ipairs(patches) do
    patch.start1 = patch.start1 + paddingLength
    patch.start2 = patch.start2 + paddingLength
  end

  -- Add some padding on start of first diff.
  local patch = patches[1]
  local diffs = patch.diffs
  local firstDiff = diffs[1]
  if (firstDiff == nil) or (firstDiff[1] ~= DIFF_EQUAL) then
    -- Add nullPadding equality.
    tinsert(diffs, 1, {DIFF_EQUAL, nullPadding})
    patch.start1 = patch.start1 - paddingLength  -- Should be 0.
    patch.start2 = patch.start2 - paddingLength  -- Should be 0.
    patch.length1 = patch.length1 + paddingLength
    patch.length2 = patch.length2 + paddingLength
  elseif (paddingLength > #firstDiff[2]) then
    -- Grow first equality.
    local extraLength = paddingLength - #firstDiff[2]
    firstDiff[2] = strsub(nullPadding, #firstDiff[2] + 1) .. firstDiff[2]
    patch.start1 = patch.start1 - extraLength
    patch.start2 = patch.start2 - extraLength
    patch.length1 = patch.length1 + extraLength
    patch.length2 = patch.length2 + extraLength
  end

  -- Add some padding on end of last diff.
  patch = patches[#patches]
  diffs = patch.diffs
  local lastDiff = diffs[#diffs]
  if (lastDiff == nil) or (lastDiff[1] ~= DIFF_EQUAL) then
    -- Add nullPadding equality.
    diffs[#diffs + 1] = {DIFF_EQUAL, nullPadding}
    patch.length1 = patch.length1 + paddingLength
    patch.length2 = patch.length2 + paddingLength
  elseif (paddingLength > #lastDiff[2]) then
    -- Grow last equality.
    local extraLength = paddingLength - #lastDiff[2]
    lastDiff[2] = lastDiff[2] .. strsub(nullPadding, 1, extraLength)
    patch.length1 = patch.length1 + extraLength
    patch.length2 = patch.length2 + extraLength
  end

  return nullPadding
end

--[[
* Look through the patches and break up any which are longer than the maximum
* limit of the match algorithm.
* Intended to be called only from within patch_apply.
* @param {Array.<_new_patch_obj>} patches Array of patch objects.
--]]
function _patch_splitMax(patches)
  local patch_size = Match_MaxBits
  local x = 1
  while true do
    local patch = patches[x]
    if patch == nil then
      return
    end
    if patch.length1 > patch_size then
      local bigpatch = patch
      -- Remove the big old patch.
      tremove(patches, x)
      x = x - 1
      local start1 = bigpatch.start1
      local start2 = bigpatch.start2
      local precontext = ''
      while bigpatch.diffs[1] do
        -- Create one of several smaller patches.
        local patch = _new_patch_obj()
        local empty = true
        patch.start1 = start1 - #precontext
        patch.start2 = start2 - #precontext
        if precontext ~= '' then
          patch.length1, patch.length2 = #precontext, #precontext
          patch.diffs[#patch.diffs + 1] = {DIFF_EQUAL, precontext}
        end
        while bigpatch.diffs[1] and (patch.length1 < patch_size-Patch_Margin) do
          local diff_type = bigpatch.diffs[1][1]
          local diff_text = bigpatch.diffs[1][2]
          if (diff_type == DIFF_INSERT) then
            -- Insertions are harmless.
            patch.length2 = patch.length2 + #diff_text
            start2 = start2 + #diff_text
            patch.diffs[#(patch.diffs) + 1] = bigpatch.diffs[1]
            tremove(bigpatch.diffs, 1)
            empty = false
          elseif (diff_type == DIFF_DELETE) and (#patch.diffs == 1)
           and (patch.diffs[1][1] == DIFF_EQUAL)
           and (#diff_text > 2 * patch_size) then
            -- This is a large deletion.  Let it pass in one chunk.
            patch.length1 = patch.length1 + #diff_text
            start1 = start1 + #diff_text
            empty = false
            patch.diffs[#patch.diffs + 1] = {diff_type, diff_text}
            tremove(bigpatch.diffs, 1)
          else
            -- Deletion or equality.
            -- Only take as much as we can stomach.
            diff_text = strsub(diff_text, 1,
            patch_size - patch.length1 - Patch_Margin)
            patch.length1 = patch.length1 + #diff_text
            start1 = start1 + #diff_text
            if (diff_type == DIFF_EQUAL) then
              patch.length2 = patch.length2 + #diff_text
              start2 = start2 + #diff_text
            else
              empty = false
            end
            patch.diffs[#patch.diffs + 1] = {diff_type, diff_text}
            if (diff_text == bigpatch.diffs[1][2]) then
              tremove(bigpatch.diffs, 1)
            else
              bigpatch.diffs[1][2]
                  = strsub(bigpatch.diffs[1][2], #diff_text + 1)
            end
          end
        end
        -- Compute the head context for the next patch.
        precontext = _diff_text2(patch.diffs)
        precontext = strsub(precontext, -Patch_Margin)
        -- Append the end context for this patch.
        local postcontext = strsub(_diff_text1(bigpatch.diffs), 1, Patch_Margin)
        if postcontext ~= '' then
          patch.length1 = patch.length1 + #postcontext
          patch.length2 = patch.length2 + #postcontext
          if patch.diffs[1]
              and (patch.diffs[#patch.diffs][1] == DIFF_EQUAL) then
            patch.diffs[#patch.diffs][2] = patch.diffs[#patch.diffs][2]
                .. postcontext
          else
            patch.diffs[#patch.diffs + 1] = {DIFF_EQUAL, postcontext}
          end
        end
        if not empty then
          x = x + 1
          tinsert(patches, x, patch)
        end
      end
    end
    x = x + 1
  end
end

--[[
* Emulate GNU diff's format.
* Header: @@ -382,8 +481,9 @@
* @return {string} The GNU diff string.
--]]
function _patch_appendText(patch, text)
  local coords1, coords2
  local length1, length2 = patch.length1, patch.length2
  local start1, start2 = patch.start1, patch.start2
  local diffs = patch.diffs

  if length1 == 1 then
    coords1 = start1
  else
    coords1 = ((length1 == 0) and (start1 - 1) or start1) .. ',' .. length1
  end

  if length2 == 1 then
    coords2 = start2
  else
    coords2 = ((length2 == 0) and (start2 - 1) or start2) .. ',' .. length2
  end
  text[#text + 1] = '@@ -' .. coords1 .. ' +' .. coords2 .. ' @@\n'

  local op
  -- Escape the body of the patch with %xx notation.
  for x, diff in ipairs(patch.diffs) do
    local diff_type = diff[1]
    if diff_type == DIFF_INSERT then
      op = '+'
    elseif diff_type == DIFF_DELETE then
      op = '-'
    elseif diff_type == DIFF_EQUAL then
      op = ' '
    end
    text[#text + 1] = op
        .. gsub(diffs[x][2], percentEncode_pattern, percentEncode_replace)
        .. '\n'
  end

  return text
end

-- Expose the API
local _M = {}

_M.DIFF_DELETE = DIFF_DELETE
_M.DIFF_INSERT = DIFF_INSERT
_M.DIFF_EQUAL = DIFF_EQUAL

_M.diff_main = diff_main
_M.diff_cleanupSemantic = diff_cleanupSemantic
_M.diff_cleanupEfficiency = diff_cleanupEfficiency
_M.diff_levenshtein = diff_levenshtein
_M.diff_prettyHtml = diff_prettyHtml

_M.match_main = match_main

_M.patch_make = patch_make
_M.patch_toText = patch_toText
_M.patch_fromText = patch_fromText
_M.patch_apply = patch_apply

-- Expose some non-API functions as well, for testing purposes etc.
_M.diff_commonPrefix = _diff_commonPrefix
_M.diff_commonSuffix = _diff_commonSuffix
_M.diff_commonOverlap = _diff_commonOverlap
_M.diff_halfMatch = _diff_halfMatch
_M.diff_bisect = _diff_bisect
_M.diff_cleanupMerge = _diff_cleanupMerge
_M.diff_cleanupSemanticLossless = _diff_cleanupSemanticLossless
_M.diff_text1 = _diff_text1
_M.diff_text2 = _diff_text2
_M.diff_toDelta = _diff_toDelta
_M.diff_fromDelta = _diff_fromDelta
_M.diff_xIndex = _diff_xIndex
_M.match_alphabet = _match_alphabet
_M.match_bitap = _match_bitap
_M.new_patch_obj = _new_patch_obj
_M.patch_addContext = _patch_addContext
_M.patch_splitMax = _patch_splitMax
_M.patch_addPadding = _patch_addPadding
_M.settings = settings

return _M
