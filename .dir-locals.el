((c++-mode . ((eval . (progn
                        (em-append-include-dirs (list (expand-file-name "~/src/apl/src")
                                                      "/usr/include/postgresql"))
                        (flycheck-mode 1)
                        (company-mode 1))))))
