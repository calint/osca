-- Options are automatically loaded before lazy.nvim startup
-- Default options that are always set: https://github.com/LazyVim/LazyVim/blob/main/lua/lazyvim/config/options.lua
-- Add any additional options here

local opt = vim.opt

opt.spell = true
opt.spelllang = { "en_us" }
opt.colorcolumn = "81,101,121" -- delimiter at columns
opt.tabstop = 4 -- number of spaces for tab
opt.shiftwidth = 4 -- size of indent
opt.scrolloff = 100 -- keeps window vertically centered

vim.filetype.add({
  extension = {
    baz = "baz",
  },
})
