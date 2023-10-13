module.exports = {
    env: {
        node: true,
    },
    extends      : ["eslint:recommended"],
    parserOptions: {
        ecmaVersion: "latest",
        sourceType : "module",
    },
    rules: {
        "object-curly-spacing": ["error", "always"],
        "brace-style"         : ["error", "allman"],
        quotes                : ["error", "double"],
        semi                  : ["error", "always"],
        "linebreak-style"     : ["error", "unix"],
        indent                : ["error", 4],
        "no-else-return"      : 1,
        "space-unary-ops"     : 2,
        "keyword-spacing"     : ["error", {
            "before": true,
            "after" : true,
        }],
        "arrow-spacing"      : "error",
        "comma-spacing"      : ["error", { "before": false, "after": true }],
        "space-infix-ops"    : ["error", { "int32Hint": false }],
        "space-before-blocks": ["error", { "functions": "always", "keywords": "always", "classes": "always" }],
        "no-multi-spaces"    : "error",
        "no-trailing-spaces" : "error",
        "semi-spacing"       : "error",
        "key-spacing"        : [2, {
            "singleLine": {
                "beforeColon": false,
                "afterColon" : true
            },
            "multiLine": {
                "beforeColon": false,
                "afterColon" : true,
                "align"      : "colon"
            }
        }]
    },
};
