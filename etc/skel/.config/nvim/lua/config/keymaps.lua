-- Keymaps are automatically loaded on the VeryLazy event
-- Default keymaps that are always set: https://github.com/LazyVim/LazyVim/blob/main/lua/lazyvim/config/keymaps.lua
--

local keymap = vim.keymap.set

--
-- debugger
--
local dap = require("dap")
keymap("n", "<F10>", dap.step_over, { noremap = true, silent = true, desc = "Debug: Step Over" })
keymap("n", "<F11>", dap.step_into, { noremap = true, silent = true, desc = "Debug: Step Into" })
keymap("n", "<F11>", dap.step_out, { noremap = true, silent = true, desc = "Debug: Step Out" })
