package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;
import org.opencv.test.OpenCVTestRunner;
import org.opencv.features2d.AKAZE;

public class AKAZEDescriptorExtractorTest extends OpenCVTestCase {

    AKAZE extractor;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        extractor = AKAZE.create(); // default (5,0,3,0.001f,4,4,1)
    }

    public void testCreate() {
        assertNotNull(extractor);
    }

    public void testDetectListOfMatListOfListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectListOfMatListOfListOfKeyPointListOfMat() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPointMat() {
        fail("Not yet implemented");
    }

    public void testEmpty() {
        fail("Not yet implemented");
    }

    public void testRead() {
        String filename = OpenCVTestRunner.getTempFileName("xml");
        writeFile(filename, "<?xml version=\"1.0\"?>\n<opencv_storage>\n<format>3</format>\n<name>Feature2D.AKAZE</name>\n<descriptor>4</descriptor>\n<descriptor_channels>2</descriptor_channels>\n<descriptor_size>32</descriptor_size>\n<threshold>0.002</threshold>\n<octaves>3</octaves>\n<sublevels>5</sublevels>\n<diffusivity>2</diffusivity>\n</opencv_storage>\n");

        extractor.read(filename);

        assertEquals(4, extractor.getDescriptorType());
        assertEquals(2, extractor.getDescriptorChannels());
        assertEquals(32, extractor.getDescriptorSize());
        assertEquals((float) 0.002, extractor.getThreshold());
        assertEquals(3, extractor.getNOctaves());
        assertEquals(5, extractor.getNOctaveLayers());
        assertEquals(2, extractor.getDiffusivity());
    }

    public void testReadYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");
        writeFile(filename, "%YAML:1.0\n---\nformat: 3\nname: \"Feature2D.AKAZE\"\ndescriptor: 4\ndescriptor_channels: 2\ndescriptor_size: 32\nthreshold: 0.002\noctaves: 3\nsublevels: 5\ndiffusivity: 2\n");

        extractor.read(filename);

        assertEquals(4, extractor.getDescriptorType());
        assertEquals(2, extractor.getDescriptorChannels());
        assertEquals(32, extractor.getDescriptorSize());
        assertEquals((float) 0.002, extractor.getThreshold());
        assertEquals(3, extractor.getNOctaves());
        assertEquals(5, extractor.getNOctaveLayers());
        assertEquals(2, extractor.getDiffusivity());
    }

    public void testWrite() {
        String filename = OpenCVTestRunner.getTempFileName("xml");

        extractor.write(filename);

        String truth = "<?xml version=\"1.0\"?>\n<opencv_storage>\n<format>3</format>\n<name>Feature2D.AKAZE</name>\n<descriptor>5</descriptor>\n<descriptor_channels>3</descriptor_channels>\n<descriptor_size>0</descriptor_size>\n<threshold>1.0000000474974513e-03</threshold>\n<octaves>4</octaves>\n<sublevels>4</sublevels>\n<diffusivity>1</diffusivity>\n</opencv_storage>\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

    public void testWriteYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        extractor.write(filename);

        String truth = "%YAML:1.0\n---\nformat: 3\nname: \"Feature2D.AKAZE\"\ndescriptor: 5\ndescriptor_channels: 3\ndescriptor_size: 0\nthreshold: 1.0000000474974513e-03\noctaves: 4\nsublevels: 4\ndiffusivity: 1\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
    }

}
