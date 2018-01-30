--[[
Copyright 2010 Google Inc.
All Rights Reserved.

Diff Speed Test

fraser@google.com
--]]

package.path = package.path .. ';../?.lua'
local dmp = require 'diff_match_patch'

function main()
  text1 = readlines('speedtest1.txt')
  text2 = readlines('speedtest2.txt')

  dmp.settings{ Diff_Timeout=0 }

  -- Execute one reverse diff as a warmup.
  dmp.diff_main(text2, text1, false)
  collectgarbage('collect')

  start_time = os.clock()
  dmp.diff_main(text1, text2, false)
  end_time = os.clock()
  print('Elapsed time: ' .. (end_time - start_time))
end

function readlines(filename)
  f = io.open(filename, 'rb')
  text = f:read('*a')
  f:close()
  return text
end

main()
