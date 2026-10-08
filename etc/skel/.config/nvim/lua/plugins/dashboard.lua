return {
  {
    "folke/snacks.nvim",
    opts = {
      dashboard = {
        enabled = false,
      },
      picker = {
        sources = {
          explorer = {
            title = "",
            -- no top border on the input so that the empty title row is gone
            layout = {
              preview = false,
              layout = {
                backdrop = false,
                width = 40,
                min_width = 40,
                height = 0,
                position = "left",
                border = "none",
                box = "vertical",
                { win = "input", height = 1, border = "none" },
                { win = "list", border = "none" },
              },
            },
          },
        },
      },
    },
  },
}
