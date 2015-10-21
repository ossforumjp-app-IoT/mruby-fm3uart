MRuby::Gem::Specification.new('mruby-fm3uart') do |spec|
  spec.license = ''
  spec.author  = 'lsi-dev'
  spec.summary = 'fm3 uart class'

  spec.mruby.cc.include_paths << '#{build.root}/src'
end
