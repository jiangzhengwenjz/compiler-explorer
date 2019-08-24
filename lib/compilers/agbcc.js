const BaseCompiler = require('../base-compiler');

class AgbccCompiler extends BaseCompiler {
    optionsForFilter(filters, outputFilename) {
        let options = ['-o', this.filename(outputFilename)];
        if (this.compiler.intelAsm && filters.intel && !filters.binary) {
            options = options.concat(this.compiler.intelAsm.split(" "));
        }
        return options;
    }
}

module.exports = AgbccCompiler;

