return {
  {
    "sainnhe/everforest",
  },
  {
    "LazyVim/LazyVim",
    opts = {
      colorscheme = "everforest",
    },
    init = function()
      -- locals a little brighter than the default foreground (used by
      -- parameters) so that the two can be told apart
      local function set_local_variable_colors()
        local fg = vim.o.background == "dark" and "#ece1c3" or "#3f4b52"
        vim.api.nvim_set_hl(0, "@variable", { fg = fg })
        vim.api.nvim_set_hl(0, "@lsp.type.variable", { fg = fg })

        -- parameters get their own hue and italics, which is what actually
        -- separates them from locals
        local param_fg = vim.o.background == "dark" and "#e69875" or "#f57d26"
        vim.api.nvim_set_hl(0, "@variable.parameter", { fg = param_fg, italic = true })
        vim.api.nvim_set_hl(0, "@lsp.type.parameter", { fg = param_fg, italic = true })
      end

      vim.api.nvim_create_autocmd("ColorScheme", {
        pattern = "everforest",
        callback = set_local_variable_colors,
      })
    end,
  },
}
