return {
  {
    "sainnhe/everforest",
  },
  {
    -- the bufferline filler above the explorer uses the darkened side pane
    -- background instead of a guessed, undarkened one
    "akinsho/bufferline.nvim",
    opts = {
      options = {
        offsets = {
          {
            filetype = "neo-tree",
            text = "Neo-tree",
            highlight = "SidePaneNormal",
            text_align = "left",
          },
          {
            filetype = "snacks_layout_box",
            highlight = "SidePaneNormal",
          },
        },
      },
    },
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

        -- globals get a muted steel blue, a little bluer and duller than the
        -- teal blue everforest uses for fields and properties
        local global_fg = vim.o.background == "dark" and "#7a9bb5" or "#4a7a9c"
        vim.api.nvim_set_hl(0, "@lsp.typemod.variable.globalScope", { fg = global_fg })
        vim.api.nvim_set_hl(0, "@lsp.typemod.variable.static", { fg = global_fg })

        -- baz highlights.scm marks file level 'dat', 'var' and 'let' as
        -- '@module', which everforest colors like '@type'
        vim.api.nvim_set_hl(0, "@module.baz", { fg = global_fg })
      end

      -- explorer (left) and claude (right) panes get a background a little
      -- darker than the editor so that the editor stands out
      local function set_side_pane_colors()
        local normal = vim.api.nvim_get_hl(0, { name = "Normal", link = false })
        if not normal.bg then
          return
        end

        local factor = 0.85
        local r = math.floor(bit.rshift(normal.bg, 16) % 256 * factor)
        local g = math.floor(bit.rshift(normal.bg, 8) % 256 * factor)
        local b = math.floor(normal.bg % 256 * factor)
        local bg = r * 65536 + g * 256 + b

        vim.api.nvim_set_hl(0, "SidePaneNormal", { bg = bg })
        for _, group in ipairs({
          "SnacksPickerList",
          "SnacksPickerInput",
          "SnacksPickerPreview",
          "SnacksPickerBox",
        }) do
          vim.api.nvim_set_hl(0, group, { bg = bg })
          vim.api.nvim_set_hl(0, group .. "Normal", { bg = bg })
          vim.api.nvim_set_hl(0, group .. "Border", { bg = bg, fg = bg })
        end
      end

      vim.api.nvim_create_autocmd("ColorScheme", {
        pattern = "everforest",
        callback = function()
          set_local_variable_colors()
          set_side_pane_colors()
        end,
      })
    end,
  },
}
