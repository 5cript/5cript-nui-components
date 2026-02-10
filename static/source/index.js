(() => {
    function detectColorScheme(){
        var theme="dark";    //default to dark

        //local storage is used to override OS theme settings
        if(!window.matchMedia) {
            //matchMedia method not supported
        } else if(window.matchMedia("(prefers-color-scheme: light)").matches) {
            //OS theme setting detected as light
            var theme = "light";
        }

        //dark theme preferred, set document with a `data-theme` attribute
        if (theme=="dark") {
            document.documentElement.setAttribute("data-theme", "dark");
        } else {
            document.documentElement.setAttribute("data-theme", "light");
        }
    }
    detectColorScheme();
})();
