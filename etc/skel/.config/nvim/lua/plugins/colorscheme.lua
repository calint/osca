return {
  "folke/tokyonight.nvim",
  style = "moon",
  opts = {
    on_highlights = function(hl, colors)
      hl["@lsp.typemod.variable.fileScope.cpp"] = {
        fg = colors.cyan,
        bold = true,
      }
      hl["@lsp.typemod.variable.globalScope.cpp"] = {
        fg = colors.cyan,
        bold = true,
      }
    end,
  },
}
